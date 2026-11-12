#ifndef PluginVersion
  #define PluginVersion "1.1.4"
#endif
#ifndef StageDir
  #error StageDir must point to the verified Windows release staging folder.
#endif
[Setup]
AppId={{E57AF943-CDB4-4CA6-B29C-0CD919EE92F7}
AppName=NMG
AppVersion={#PluginVersion}
AppPublisher=NMG
DefaultDirName={autopf}\NMG
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#StageDir}
OutputBaseFilename=NMG-{#PluginVersion}-Windows-x64-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName=NMG {#PluginVersion}
CloseApplications=yes
CloseApplicationsFilter=*.exe,*.dll,*.vst3
RestartApplications=no
SetupLogging=yes
InfoBeforeFile={#StageDir}\INSTALLATION.txt
[Files]
Source: "{#StageDir}\VST3\NMG.vst3\*"; DestDir: "{commoncf64}\VST3\NMG.vst3"; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#StageDir}\INSTALLATION.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#StageDir}\MicrosoftEdgeWebview2Setup.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall
[Run]
Filename: "{tmp}\MicrosoftEdgeWebview2Setup.exe"; Parameters: "/silent /install"; StatusMsg: "Installing Microsoft WebView2 Runtime if needed..."; Flags: waituntilterminated; Check: NeedsWebView2
[Code]
function NeedsWebView2: Boolean;
var Version: String;
begin
 Result := not ((RegQueryStringValue(HKLM32, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0')) or
  (RegQueryStringValue(HKCU, 'SOFTWARE\Microsoft\EdgeUpdate\Clients\{F3017226-FE2A-4295-8BDF-00C3A9A7E4C5}', 'pv', Version) and (Version <> '') and (Version <> '0.0.0.0')));
end;
