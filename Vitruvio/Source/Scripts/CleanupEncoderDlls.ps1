param ([string]$PluginDir)
function Remove-Old-File($Dir, $Name, $FileEnding)
{
    Get-ChildItem -Force -File -Path $Dir | Where-Object { $_.Name -match "$Name(-\d{4})?\.$FileEnding" } -ErrorAction SilentlyContinue | Sort-Object CreationTime -Descending | Select-Object -Skip 1 | Remove-Item -ErrorAction SilentlyContinue
}

function Remove-Old-Build($Dir, $Name)
{
    Remove-Old-File -Dir $Dir -Name $Name -FileEnding "dll"
    Remove-Old-File -Dir $Dir -Name $Name -FileEnding "pdb"
}

Write-Host "Cleanup Encoder dlls"

$BinariesDir="$PluginDir\Binaries\Win64"

Remove-Old-Build -Dir $BinariesDir -Name "UE4Editor-UnrealGeometryEncoder"
Remove-Old-Build -Dir $BinariesDir -Name "UE4Editor-Vitruvio"
Remove-Old-Build -Dir $BinariesDir -Name "UE4Editor-VitruvioEditor"
