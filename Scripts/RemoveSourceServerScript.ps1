# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Remove IndexGitSource.ps1 from PostBuildEvent
# Source Link를 사용하므로 기존 Source Server 스크립트 제거
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

param(
    [string]$VcxprojPath = "C:\Users\Jungle\Desktop\GTL_Week12\Mundi\Mundi.vcxproj"
)

Write-Host "[RemoveSourceServer] Removing IndexGitSource.ps1 from $VcxprojPath" -ForegroundColor Cyan

# 파일 읽기
$content = Get-Content $VcxprojPath -Raw

# IndexGitSource.ps1 라인 제거
$pattern = 'powershell\.exe[^\r\n]*IndexGitSource\.ps1[^\r\n]*[\r\n]+'
$newContent = $content -replace $pattern, ''

# 파일 저장
$newContent | Set-Content $VcxprojPath -NoNewline

Write-Host "[RemoveSourceServer] IndexGitSource.ps1 calls removed successfully!" -ForegroundColor Green
