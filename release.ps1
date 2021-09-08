# Menu is at the end of the file

function Clear-ProjectDirectory
{
Remove-Item -Path .\x86 -Recurse
}

function New-GeneralRelease
{
mkdir "Tools"
mkdir "Tools\Audio"

$buildCmd = "/c `"`"%VS2019INSTALLDIR%\Common7\Tools\VsDevCmd.bat`" & msbuild FFXW32.sln -m -t:Build -p:Configuration=Release;Platform=x86`""
(Start-Process "cmd.exe" $buildCmd -PassThru) | Wait-Process

$readMeInfo = @" 
#
# NOTICE
#
# While FaceFXWrapper produces LIP files for the 64-bit Creation Kit, it
# has not been validated for correctness. Use at your own risk.
#
# 'FaceFXWrapper.exe' must be in the directory '<GAME_DIR>\Tools\Audio'.
#
# See README.md in the repository at https://github.com/Nukem9/FaceFXWrapper for more usage information.
# 
# REQUIREMENTS
#
# FonixData.cdf is a proprietary file required for FaceFX. You can copy FonixData
# from the Fallout 3, Fallout 4, or Skyrim LE Creation Kit files. It must be placed
# in '<GAME_DIR>\Data\Sound\Voice\Processing'.
#
# FonixData.cdf may also be obtained from NexusMods: https://www.nexusmods.com/newvegas/mods/61248/ or 
# https://www.nexusmods.com/skyrimspecialedition/mods/40971
#
"@

$readMeInfo | Out-File -FilePath "Tools\Audio\README.txt" -Encoding ASCII

Set-Location "x86\Release"
Copy-Item "FaceFXWrapper.exe" "..\..\Tools\Audio\FaceFXWrapper.exe"
Set-Location ..
Set-Location ..

[Reflection.Assembly]::LoadWithPartialName("System.IO.Compression.FileSystem")
$compression = [System.IO.Compression.CompressionLevel]::Optimal
[System.IO.Compression.ZipFile]::CreateFromDirectory((Get-Location).Path + "\Tools", (Get-Location).Path + "\FaceFXWrapper X.zip", $compression, $true) # Include base dir

Remove-Item -Path "Tools" -Recurse
}

# Check for params passed on the command line
$argInput = $args[0]

if ([string]::IsNullOrWhiteSpace($argInput)) {
    Write-Host "==================================="
    Write-Host "1: Clean project directory"
    Write-Host "2: Create release build archives"
    Write-Host "==================================="

    $argInput = Read-Host "Selection"
}

switch ($argInput)
{
    '1' {
        Clear-Host
        Clear-ProjectDirectory
    } '2' {
        Clear-Host
        New-GeneralRelease
    }
}

exit
