# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 심볼 서버 자동 업로드 스크립트
# 빌드 후 이벤트로 실행되어 PDB를 심볼 서버에 자동 업로드
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

param(
    [string]$ExePath,       # EXE 파일 경로
    [string]$PdbPath,       # PDB 파일 경로
    [string]$SymbolServer,  # 심볼 서버 경로 (예: \\PC-NAME\SymbolServer)
    [string]$ProductName = "Mundi",
    [string]$Version = "1.0.0"
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 입력 검증
if (-not (Test-Path $ExePath)) {
    Write-Host "[SymbolUpload] EXE not found: $ExePath" -ForegroundColor Red
    exit 0  # 경고만 표시하고 빌드는 성공
}

if (-not (Test-Path $PdbPath)) {
    Write-Host "[SymbolUpload] PDB not found: $PdbPath" -ForegroundColor Yellow
    exit 0
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# symstore.exe 찾기
$SymstorePaths = @(
    "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\symstore.exe",
    "C:\Program Files (x86)\Windows Kits\10\Debuggers\x86\symstore.exe",
    "C:\Program Files\Debugging Tools for Windows (x64)\symstore.exe"
)

$SymstoreExe = $null
foreach ($Path in $SymstorePaths) {
    if (Test-Path $Path) {
        $SymstoreExe = $Path
        break
    }
}

if (-not $SymstoreExe) {
    Write-Host "[SymbolUpload] symstore.exe not found (Windows SDK not installed?)" -ForegroundColor Yellow
    exit 0
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# PDB GUID 추출 함수
function Get-PdbGuid {
    param([string]$ExePath)

    try {
        $Bytes = [System.IO.File]::ReadAllBytes($ExePath)

        # DOS Header (e_lfanew at offset 0x3C)
        $PeOffset = [BitConverter]::ToInt32($Bytes, 0x3C)

        # NT Header
        $NumberOfSections = [BitConverter]::ToUInt16($Bytes, $PeOffset + 6)
        $OptionalHeaderOffset = $PeOffset + 24

        # Debug Directory RVA (offset 0xA8 in Optional Header for x64)
        $DebugDirRva = [BitConverter]::ToUInt32($Bytes, $OptionalHeaderOffset + 0xA8)

        if ($DebugDirRva -eq 0) { return $null }

        # Section Headers 시작 (Optional Header 이후)
        $SectionHeadersOffset = $OptionalHeaderOffset + 240  # x64 Optional Header 크기

        # Debug Directory의 파일 오프셋 찾기
        $FileOffset = 0
        for ($i = 0; $i -lt $NumberOfSections; $i++) {
            $SectionOffset = $SectionHeadersOffset + ($i * 40)
            $VirtualAddress = [BitConverter]::ToUInt32($Bytes, $SectionOffset + 12)
            $SizeOfRawData = [BitConverter]::ToUInt32($Bytes, $SectionOffset + 16)
            $PointerToRawData = [BitConverter]::ToUInt32($Bytes, $SectionOffset + 20)

            if ($DebugDirRva -ge $VirtualAddress -and $DebugDirRva -lt ($VirtualAddress + $SizeOfRawData)) {
                $FileOffset = $PointerToRawData + ($DebugDirRva - $VirtualAddress)
                break
            }
        }

        if ($FileOffset -eq 0) { return $null }

        # Debug Directory
        $PointerToRawData = [BitConverter]::ToUInt32($Bytes, $FileOffset + 24)

        # RSDS Header
        $Signature = [BitConverter]::ToUInt32($Bytes, $PointerToRawData)
        if ($Signature -ne 0x53445352) { return $null }  # 'RSDS'

        # GUID 추출 (16 bytes)
        $GuidBytes = $Bytes[($PointerToRawData + 4)..($PointerToRawData + 19)]
        $Guid = [System.Guid]::new($GuidBytes)

        # Age 추출
        $Age = [BitConverter]::ToUInt32($Bytes, $PointerToRawData + 20)

        # GUID를 대문자 16진수 문자열로 변환 (하이픈 제거)
        $GuidStr = $Guid.ToString("N").ToUpper()

        return "$GuidStr$Age"
    }
    catch {
        return $null
    }
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 심볼 서버에 이미 존재하는지 확인
$PdbFileName = [System.IO.Path]::GetFileName($PdbPath)
$ExeFileName = [System.IO.Path]::GetFileName($ExePath)
$PdbSignature = Get-PdbGuid -ExePath $ExePath

$PdbExists = $false
$ExeExists = $false

if ($PdbSignature) {
    $ServerPdbPath = Join-Path $SymbolServer "$PdbFileName\$PdbSignature\$PdbFileName"
    $ServerExePath = Join-Path $SymbolServer "$ExeFileName\$PdbSignature\$ExeFileName"

    if (Test-Path $ServerPdbPath) {
        Write-Host "[SymbolUpload] PDB already exists on server: $PdbFileName" -ForegroundColor Green
        $PdbExists = $true
    }

    if (Test-Path $ServerExePath) {
        Write-Host "[SymbolUpload] EXE already exists on server: $ExeFileName" -ForegroundColor Green
        $ExeExists = $true
    }

    if ($PdbExists -and $ExeExists) {
        Write-Host "[SymbolUpload] Both PDB and EXE already exist on server" -ForegroundColor Green
        exit 0
    }
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 심볼 서버에 업로드
Write-Host "[SymbolUpload] Uploading symbols to server..." -ForegroundColor Cyan

# PDB 파일 업로드
if (-not $PdbExists) {
    $PdbArgs = @(
        "add",
        "/f", "`"$PdbPath`"",
        "/s", "`"$SymbolServer`"",
        "/t", "`"$ProductName`"",
        "/v", "`"$Version`"",
        "/compress"
    )

    $Process = Start-Process -FilePath $SymstoreExe -ArgumentList $PdbArgs -Wait -PassThru -NoNewWindow

    if ($Process.ExitCode -eq 0) {
        Write-Host "[SymbolUpload] PDB upload successful: $PdbFileName" -ForegroundColor Green
    } else {
        Write-Host "[SymbolUpload] PDB upload failed with code $($Process.ExitCode)" -ForegroundColor Red
        exit 0  # 실패해도 빌드는 성공
    }
}

# EXE 파일 업로드
if (-not $ExeExists) {
    Write-Host "[SymbolUpload] Uploading executable to server..." -ForegroundColor Cyan

    $ExeArgs = @(
        "add",
        "/f", "`"$ExePath`"",
        "/s", "`"$SymbolServer`"",
        "/t", "`"$ProductName`"",
        "/v", "`"$Version`"",
        "/compress"
    )

    $Process = Start-Process -FilePath $SymstoreExe -ArgumentList $ExeArgs -Wait -PassThru -NoNewWindow

    if ($Process.ExitCode -eq 0) {
        Write-Host "[SymbolUpload] EXE upload successful: $ExeFileName" -ForegroundColor Green
    } else {
        Write-Host "[SymbolUpload] EXE upload failed with code $($Process.ExitCode)" -ForegroundColor Red
    }
}

exit 0  # 실패해도 빌드는 성공
