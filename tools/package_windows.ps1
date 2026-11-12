$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root
$version = [regex]::Match((Get-Content CMakeLists.txt -Raw),'project\(NMG VERSION ([0-9.]+)').Groups[1].Value
$bundle = Join-Path $root 'build-win/NMG_artefacts/Release/VST3/NMG.vst3'
$binary = Join-Path $bundle 'Contents/x86_64-win/NMG.vst3'
if (!(Test-Path $binary)) { throw 'Real Windows VST3 binary is missing. No package will be created.' }
$bytes = [IO.File]::ReadAllBytes($binary)
$pe = [BitConverter]::ToInt32($bytes, 0x3c)
if ($bytes[0] -ne 0x4d -or $bytes[1] -ne 0x5a -or [BitConverter]::ToUInt32($bytes,$pe) -ne 0x4550 -or [BitConverter]::ToUInt16($bytes,$pe+4) -ne 0x8664) { throw 'Binary is not x64 Windows PE.' }
$stage = Join-Path $root "release/windows/stage-$version"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item "$stage/VST3" -ItemType Directory -Force | Out-Null
Copy-Item $bundle "$stage/VST3/NMG.vst3" -Recurse
Copy-Item docs/WINDOWS-INSTALLATION.txt "$stage/INSTALLATION.txt"
# Official Microsoft Evergreen bootstrapper; installed only when Runtime is absent.
Invoke-WebRequest 'https://go.microsoft.com/fwlink/p/?LinkId=2124703' -OutFile "$stage/MicrosoftEdgeWebview2Setup.exe"
$signature = Get-AuthenticodeSignature "$stage/MicrosoftEdgeWebview2Setup.exe"
if ($signature.Status -ne 'Valid' -or $signature.SignerCertificate.Subject -notmatch 'Microsoft Corporation') { throw 'Microsoft bootstrapper signature verification failed.' }
$compiler = Join-Path ${env:ProgramFiles(x86)} 'Inno Setup 6/ISCC.exe'
if (!(Test-Path $compiler)) {
  choco install innosetup --yes --no-progress
  if ($LASTEXITCODE) { throw 'Inno Setup installation failed' }
}
& $compiler "/DPluginVersion=$version" "/DStageDir=$stage" Installer/windows/NMG.iss
if ($LASTEXITCODE) { throw 'Installer compilation failed' }
$setup = Join-Path $stage "NMG-$version-Windows-x64-Setup.exe"
if (!(Test-Path $setup)) { throw 'Installer was not produced' }
$manifest = Get-ChildItem $stage -Recurse -File | ForEach-Object { (Get-FileHash $_.FullName -Algorithm SHA256).Hash + '  ' + $_.FullName.Substring($stage.Length + 1) }
$manifest | Set-Content "$stage/SHA256SUMS.txt"
$archive = Join-Path $root "release/windows/NMG-$version-Windows-x64.zip"
Compress-Archive -Path "$stage/*" -DestinationPath $archive -Force
Write-Host "Created $archive"
