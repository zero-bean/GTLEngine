# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Git Source Indexing Script
# PDB에 Git 커밋 정보를 삽입하여 동료가 DMP 분석 시 정확한 소스 매칭
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

param(
    [string]$PdbPath,          # PDB 파일 경로
    [string]$SourceRoot        # 소스 코드 루트 디렉토리
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 입력 검증
if (-not (Test-Path $PdbPath)) {
    Write-Host "[GitIndex] PDB not found: $PdbPath" -ForegroundColor Yellow
    exit 0  # 실패해도 빌드는 성공
}

if (-not (Test-Path $SourceRoot)) {
    Write-Host "[GitIndex] Source root not found: $SourceRoot" -ForegroundColor Yellow
    exit 0
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 도구 경로 설정
$SrcsrvPath = "C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\srcsrv"
$SrctoolExe = Join-Path $SrcsrvPath "srctool.exe"
$PdbstrExe = Join-Path $SrcsrvPath "pdbstr.exe"

if (-not (Test-Path $SrctoolExe)) {
    Write-Host "[GitIndex] srctool.exe not found" -ForegroundColor Yellow
    exit 0
}

if (-not (Test-Path $PdbstrExe)) {
    Write-Host "[GitIndex] pdbstr.exe not found" -ForegroundColor Yellow
    exit 0
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Git 정보 추출
Push-Location $SourceRoot

# 현재 커밋 해시
$CommitHash = git rev-parse HEAD 2>$null
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrEmpty($CommitHash)) {
    Write-Host "[GitIndex] Failed to get Git commit hash (not a git repo?)" -ForegroundColor Yellow
    Pop-Location
    exit 0
}

# Git 원격 저장소 URL
$GitRepoUrl = git config --get remote.origin.url 2>$null
if ([string]::IsNullOrEmpty($GitRepoUrl)) {
    $GitRepoUrl = "LOCAL_REPO"
}

Pop-Location

Write-Host "[GitIndex] Commit: $CommitHash" -ForegroundColor Cyan
Write-Host "[GitIndex] Repo: $GitRepoUrl" -ForegroundColor Cyan

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# PDB에서 소스 파일 목록 추출
Write-Host "[GitIndex] Extracting source files from PDB..." -ForegroundColor Cyan

$SourceFiles = & $SrctoolExe -r $PdbPath 2>&1 | Where-Object { $_ -notmatch "source files are indexed" }
if ($SourceFiles.Count -eq 0) {
    Write-Host "[GitIndex] No source files found in PDB" -ForegroundColor Yellow
    exit 0
}

Write-Host "[GitIndex] Found $($SourceFiles.Count) source files" -ForegroundColor Cyan

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Source Server Stream 생성
$SrcSrvStream = @"
SRCSRV: ini ------------------------------------------------
VERSION=2
VERCTRL=git
SRCSRV: variables ------------------------------------------
GIT_COMMIT=$CommitHash
GIT_REPO=$GitRepoUrl
SRCSRVTRG=%GIT_REPO%/raw/%GIT_COMMIT%/%var2%
SRCSRV: source files ---------------------------------------
"@

# 소스 파일 매핑 추가
$SourceRootNormalized = $SourceRoot.TrimEnd('\').ToLower()
foreach ($File in $SourceFiles) {
    $FileNormalized = $File.ToLower()

    # 프로젝트 루트 기준 상대 경로 계산
    if ($FileNormalized.StartsWith($SourceRootNormalized)) {
        $RelativePath = $File.Substring($SourceRoot.Length).TrimStart('\')
        $RelativePath = $RelativePath.Replace('\', '/')

        # 형식: 절대경로*상대경로
        $SrcSrvStream += "$File*$RelativePath`n"
    }
}

$SrcSrvStream += "SRCSRV: end ------------------------------------------------"

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 임시 파일에 저장
$TempFile = [System.IO.Path]::GetTempFileName()
$SrcSrvStream | Out-File -FilePath $TempFile -Encoding ASCII -NoNewline

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# PDB에 Source Server 정보 삽입
Write-Host "[GitIndex] Writing source index to PDB..." -ForegroundColor Cyan

$Args = @(
    "-w",
    "-p:$PdbPath",
    "-i:$TempFile",
    "-s:srcsrv"
)

$Process = Start-Process -FilePath $PdbstrExe -ArgumentList $Args -Wait -PassThru -NoNewWindow

# 임시 파일 삭제
Remove-Item $TempFile -Force -ErrorAction SilentlyContinue

if ($Process.ExitCode -eq 0) {
    Write-Host "[GitIndex] Source indexing successful!" -ForegroundColor Green

    # 검증: PDB에서 Source Server 정보 읽기
    Write-Host "[GitIndex] Verifying..." -ForegroundColor Cyan
    $VerifyArgs = @("-r", "-p:$PdbPath", "-s:srcsrv")
    $VerifyOutput = & $PdbstrExe $VerifyArgs 2>$null

    if ($VerifyOutput -match "GIT_COMMIT") {
        Write-Host "[GitIndex] Verification successful - Git info embedded in PDB" -ForegroundColor Green
    }

    exit 0
} else {
    Write-Host "[GitIndex] Source indexing failed with code $($Process.ExitCode)" -ForegroundColor Red
    exit 0  # 실패해도 빌드는 성공
}
