param ([string]$PluginDir)
function Remove-Old($Name)
{
    Get-ChildItem -Force -File -Path "$BinariesDir\$Name" -ErrorAction SilentlyContinue | Sort-Object CreationTime -Descending | Remove-Item -ErrorAction SilentlyContinue
}

Write-Host "Cleanup Encoder dlls"

$BinariesDir="$PluginDir\Binaries\Win64"

Write-Host "$BinariesDir\Binaries\Win64\*UE4Editor*.dll"

Remove-Old("UE4Editor*.dll")
Remove-Old("UE4Editor*.pdb")

