# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
# Add /SOURCELINK linker option to vcxproj
# ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

param(
    [string]$VcxprojPath = "C:\Users\Jungle\Desktop\GTL_Week12\Mundi\Mundi.vcxproj"
)

Write-Host "[SourceLink] Adding /SOURCELINK option to $VcxprojPath" -ForegroundColor Cyan

# XML 로드
[xml]$vcxproj = Get-Content $VcxprojPath

# Namespace 설정
$ns = @{ns = "http://schemas.microsoft.com/developer/msbuild/2003"}

# 모든 Link 섹션 찾기
$linkNodes = Select-Xml -Xml $vcxproj -XPath "//ns:Link" -Namespace $ns

$modified = $false

foreach ($linkNode in $linkNodes) {
    $link = $linkNode.Node

    # AdditionalOptions 찾기
    $additionalOptions = $link.SelectSingleNode("ns:AdditionalOptions", (New-Object System.Xml.XmlNamespaceManager($vcxproj.NameTable)).AddNamespace("ns", $ns.ns))

    if ($additionalOptions -ne $null) {
        $currentValue = $additionalOptions.InnerText

        # 이미 /SOURCELINK가 있는지 확인
        if ($currentValue -notmatch "/SOURCELINK") {
            $newValue = "/SOURCELINK " + $currentValue
            $additionalOptions.InnerText = $newValue
            $modified = $true
            Write-Host "[SourceLink] Added /SOURCELINK to configuration" -ForegroundColor Green
        } else {
            Write-Host "[SourceLink] /SOURCELINK already exists in this configuration" -ForegroundColor Yellow
        }
    } else {
        # AdditionalOptions가 없으면 새로 생성
        $newAdditionalOptions = $vcxproj.CreateElement("AdditionalOptions", $ns.ns)
        $newAdditionalOptions.InnerText = "/SOURCELINK %(AdditionalOptions)"
        $link.AppendChild($newAdditionalOptions) | Out-Null
        $modified = $true
        Write-Host "[SourceLink] Created AdditionalOptions with /SOURCELINK" -ForegroundColor Green
    }
}

if ($modified) {
    # 저장
    $vcxproj.Save($VcxprojPath)
    Write-Host "[SourceLink] vcxproj file updated successfully!" -ForegroundColor Green
} else {
    Write-Host "[SourceLink] No changes needed" -ForegroundColor Cyan
}
