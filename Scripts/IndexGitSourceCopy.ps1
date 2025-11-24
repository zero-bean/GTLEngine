# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Git Source Indexing with Direct File Copy
# 소스 파일을 심볼 서버에 직접 복사하여 제공
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

param(
    [string]$PdbPath,          # PDB 파일 경로
    [string]$SourceRoot,       # 소스 코드 루트 디렉토리
    [string]$SymbolServer      # 심볼 서버 경로
)

# 경로 정규화 (큰따옴표, 작은따옴표 모두 제거)
$PdbPath = $PdbPath.Trim('"', "'").TrimEnd('\')
$SourceRoot = $SourceRoot.Trim('"', "'").TrimEnd('\')
$SymbolServer = $SymbolServer.Trim('"', "'").TrimEnd('\')

# 입력 검증
if (-not (Test-Path $PdbPath)) {
    Write-Host "[GitIndex] PDB not found: $PdbPath" -ForegroundColor Yellow
    exit 0
}

if (-not (Test-Path $SourceRoot)) {
    Write-Host "[GitIndex] Source root not found: $SourceRoot" -ForegroundColor Yellow
    exit 0
}

# 도구 경로 설정
$SrcsrvPath = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\srcsrv"
$SrctoolExe = Join-Path $SrcsrvPath "srctool.exe"
$PdbstrExe = Join-Path $SrcsrvPath "pdbstr.exe"

if (-not (Test-Path $SrctoolExe) -or -not (Test-Path $PdbstrExe)) {
    Write-Host "[GitIndex] srcsrv tools not found" -ForegroundColor Yellow
    exit 0
}

# Git 정보 추출
Push-Location $SourceRoot
$CommitHash = git rev-parse HEAD 2>$null
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrEmpty($CommitHash)) {
    Write-Host "[GitIndex] Failed to get Git commit hash" -ForegroundColor Yellow
    Pop-Location
    exit 0
}
$GitRepoUrl = git config --get remote.origin.url 2>$null
Pop-Location

Write-Host "[GitIndex] Commit: $CommitHash" -ForegroundColor Cyan
Write-Host "[GitIndex] Repo: $GitRepoUrl" -ForegroundColor Cyan

# PDB에서 소스 파일 목록 추출
Write-Host "[GitIndex] Extracting source files from PDB..." -ForegroundColor Cyan
$SourceFiles = & $SrctoolExe -r $PdbPath 2>&1 | Where-Object { $_ -notmatch "source files are indexed" }
if ($SourceFiles.Count -eq 0) {
    Write-Host "[GitIndex] No source files found in PDB" -ForegroundColor Yellow
    exit 0
}
Write-Host "[GitIndex] Found $($SourceFiles.Count) source files" -ForegroundColor Cyan

# 심볼 서버에 소스 파일 복사 디렉토리 생성
$SourceCacheDir = Join-Path $SymbolServer "Sources\$CommitHash"
if (-not (Test-Path $SourceCacheDir)) {
    try {
        New-Item -ItemType Directory -Path $SourceCacheDir -Force -ErrorAction Stop | Out-Null
        Write-Host "[GitIndex] Created source cache: $SourceCacheDir" -ForegroundColor Green
    } catch {
        Write-Host "[GitIndex] Failed to create source cache directory" -ForegroundColor Red
        exit 0
    }
}

# 소스 파일 복사 및 매핑 생성
Write-Host "[GitIndex] Copying source files to symbol server..." -ForegroundColor Cyan
$SrcSrvStream = @"
SRCSRV: ini ------------------------------------------------
VERSION=2
VERCTRL=http
SRCSRV: variables ------------------------------------------
GIT_COMMIT=$CommitHash
GIT_REPO=$GitRepoUrl
SRCSRVTRG=$SymbolServer\Sources\%var2%\%var3%
SRCSRV: source files ---------------------------------------
"@

$SourceRootNormalized = $SourceRoot.TrimEnd('\').ToLower()
$CopiedCount = 0

foreach ($File in $SourceFiles) {
    $FileNormalized = $File.ToLower()

    if ($FileNormalized.StartsWith($SourceRootNormalized)) {
        $RelativePath = $File.Substring($SourceRoot.Length).TrimStart('\')
        $RelativePathUnix = $RelativePath.Replace('\', '/')

        # 대상 경로
        $DestPath = Join-Path $SourceCacheDir $RelativePath
        $DestDir = Split-Path $DestPath -Parent

        # 디렉토리 생성 및 파일 복사
        if (Test-Path $File) {
            try {
                if (-not (Test-Path $DestDir)) {
                    New-Item -ItemType Directory -Path $DestDir -Force -ErrorAction Stop | Out-Null
                }
                Copy-Item $File $DestPath -Force -ErrorAction Stop
                $CopiedCount++
            } catch {
                Write-Host "[GitIndex] Failed to copy: $File" -ForegroundColor Yellow
            }
        }

        # Source Server 매핑 추가 (형식: 절대경로*커밋*상대경로)
        $SrcSrvStream += "$File*$CommitHash*$RelativePathUnix`n"
    }
}

Write-Host "[GitIndex] Copied $CopiedCount files to symbol server" -ForegroundColor Green

$SrcSrvStream += "SRCSRV: end ------------------------------------------------"

# PDB에 Source Server 정보 삽입
Write-Host "[GitIndex] Writing source index to PDB..." -ForegroundColor Cyan
$TempFile = [System.IO.Path]::GetTempFileName()
$SrcSrvStream | Out-File -FilePath $TempFile -Encoding ASCII -NoNewline

$Args = @("-w", "-p:$PdbPath", "-i:$TempFile", "-s:srcsrv")
$Process = Start-Process -FilePath $PdbstrExe -ArgumentList $Args -Wait -PassThru -NoNewWindow
Remove-Item $TempFile -Force -ErrorAction SilentlyContinue

if ($Process.ExitCode -eq 0) {
    Write-Host "[GitIndex] Source indexing successful!" -ForegroundColor Green
    exit 0
} else {
    Write-Host "[GitIndex] Source indexing failed" -ForegroundColor Red
    exit 0
}
