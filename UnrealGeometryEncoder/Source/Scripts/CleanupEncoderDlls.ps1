param ([string]$PluginDir)
function Remove-Old($Dir, $Name)
{
    Get-ChildItem -Force -File -Path $Dir\$Name -ErrorAction SilentlyContinue | Sort-Object CreationTime -Descending | Select-Object -Skip 1 | Remove-Item -ErrorAction SilentlyContinue
}

Write-Host "Cleanup Encoder dlls"

$BinariesDir="$PluginDir\Binaries\Win64"

Remove-Old -Dir $BinariesDir -Name "UE4Editor*.dll"
Remove-Old -Dir $BinariesDir -Name "UE4Editor*.pdb"

