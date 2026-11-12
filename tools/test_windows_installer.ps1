$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
# Run only on the disposable CI machine; never uninstall an existing user copy.
if ($env:GITHUB_ACTIONS -ne 'true') { throw 'Installer smoke test requires the disposable GitHub runner.' }
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$version = [regex]::Match((Get-Content CMakeLists.txt -Raw),'project\(NMG VERSION ([0-9.]+)').Groups[1].Value
$installed = "$env:CommonProgramFiles/VST3/NMG.vst3"
$app = "$env:ProgramFiles/NMG"
if ((Test-Path $installed) -or (Test-Path $app)) { throw 'Existing installation found; smoke test will not replace it.' }
$stage = "$root/release/windows/stage-$version"
$setup = "$stage/NMG-$version-Windows-x64-Setup.exe"
$log = "$root/release/windows/install-test.log"
$proc = Start-Process $setup -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/LOG="' + $log + '"')) -Wait -PassThru
if ($proc.ExitCode -ne 0) { throw "Installer failed: $($proc.ExitCode)" }
$binary = "$installed/Contents/x86_64-win/NMG.vst3"
if (!(Test-Path $binary)) { throw 'VST3 was not installed in the standard folder.' }
if ((Get-FileHash $binary).Hash -ne (Get-FileHash "$stage/VST3/NMG.vst3/Contents/x86_64-win/NMG.vst3").Hash) { throw 'Installed VST3 differs from the tested build.' }
$uninstaller = "$app/unins000.exe"
if (!(Test-Path $uninstaller)) { throw 'Uninstaller is missing.' }
$log = "$root/release/windows/uninstall-test.log"
$proc = Start-Process $uninstaller -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/LOG="' + $log + '"')) -Wait -PassThru
if ($proc.ExitCode -ne 0 -or (Test-Path $binary)) { throw 'Uninstaller did not remove the VST3.' }
Write-Host 'PASS Windows installation, installed binary hash, and uninstallation.'
