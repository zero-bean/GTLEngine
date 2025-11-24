# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Generate Source Link JSON for PDB
# GitHub 저장소 정보를 JSON 형식으로 생성하여 PDB에 임베드
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

param(
    [string]$OutputPath,       # JSON 파일 출력 경로
    [string]$SourceRoot        # 소스 코드 루트 디렉토리
)

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 입력 검증
if ([string]::IsNullOrEmpty($OutputPath)) {
    Write-Host "[SourceLink] OutputPath is required" -ForegroundColor Red
    exit 1
}

if (-not (Test-Path $SourceRoot)) {
    Write-Host "[SourceLink] Source root not found: $SourceRoot" -ForegroundColor Yellow
    exit 1
}

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Git 정보 추출
Push-Location $SourceRoot

# 현재 커밋 해시
$CommitHash = git rev-parse HEAD 2>$null
if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrEmpty($CommitHash)) {
    Write-Host "[SourceLink] Failed to get Git commit hash (not a git repo?)" -ForegroundColor Yellow
    Pop-Location
    exit 1
}

# Git 원격 저장소 URL
$GitRepoUrl = git config --get remote.origin.url 2>$null
if ([string]::IsNullOrEmpty($GitRepoUrl)) {
    Write-Host "[SourceLink] No Git remote origin found" -ForegroundColor Yellow
    Pop-Location
    exit 1
}

Pop-Location

Write-Host "[SourceLink] Commit: $CommitHash" -ForegroundColor Cyan
Write-Host "[SourceLink] Repo: $GitRepoUrl" -ForegroundColor Cyan

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# GitHub URL 정규화
$GitHubUrl = $GitRepoUrl

# SSH 형식을 HTTPS로 변환: git@github.com:user/repo.git -> https://github.com/user/repo
if ($GitHubUrl -match "^git@github\.com:(.+)/(.+?)(\.git)?$") {
    $GitUser = $matches[1]
    $GitRepo = $matches[2]
    $GitHubUrl = "https://github.com/$GitUser/$GitRepo"
}

# HTTPS 형식에서 .git 제거
$GitHubUrl = $GitHubUrl -replace "\.git$", ""

Write-Host "[SourceLink] Normalized URL: $GitHubUrl" -ForegroundColor Cyan

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Source Link JSON 생성
# 형식: https://github.com/dotnet/sourcelink/blob/main/docs/README.md

$SourceRootNormalized = $SourceRoot.TrimEnd('\').Replace('\', '/')

$SourceLinkJson = @{
    documents = @{
        "$SourceRootNormalized/*" = "$GitHubUrl/raw/$CommitHash/*"
    }
} | ConvertTo-Json -Depth 10

# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# 출력 디렉토리 생성
$OutputDir = Split-Path $OutputPath -Parent
if (-not [string]::IsNullOrEmpty($OutputDir) -and -not (Test-Path $OutputDir)) {
    Write-Host "[SourceLink] Creating directory: $OutputDir" -ForegroundColor Yellow
    New-Item -ItemType Directory -Path $OutputDir -Force -ErrorAction Stop | Out-Null
}

# JSON 파일 저장
try {
    $SourceLinkJson | Out-File -FilePath $OutputPath -Encoding UTF8 -NoNewline -Force
    Write-Host "[SourceLink] Generated: $OutputPath" -ForegroundColor Green
} catch {
    Write-Host "[SourceLink] Failed to write JSON: $_" -ForegroundColor Red
    exit 1
}
Write-Host "[SourceLink] Content:" -ForegroundColor Cyan
Write-Host $SourceLinkJson -ForegroundColor Gray

exit 0
