param ([switch]$Enabled, [string]$PluginDir)

if (-not $Enabled) {
    exit
}

Write-Host "Uninstalling PRT"

$ModuleDir="$PluginDir\Source\ThirdParty\PRT"
$BinariesDir="$PluginDir\Binaries\Win64"

Remove-Item -Path $BinariesDir\* -Exclude UE4*

Remove-Item -Path $ModuleDir\bin\ -Recurse
Remove-Item -Path $ModuleDir\lib\ -Recurse