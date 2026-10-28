#ifndef MyAppVersion
  #define MyAppVersion "0.3.0"
#endif

[Setup]
AppId={{8F4E2A1C-6B3D-4E91-9C70-2A1D5F8E0B44}
AppName=JimiDAW
AppVersion={#MyAppVersion}
AppPublisher=JimiDAW
DefaultDirName={autopf}\JimiDAW
DefaultGroupName=JimiDAW
DisableProgramGroupPage=yes
OutputDir=..\dist
OutputBaseFilename=JimiDAW-Setup-{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayIcon={app}\JimiDAW.exe
CloseApplications=force

[Files]
Source: "..\dist\JimiDAW.exe"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\JimiDAW"; Filename: "{app}\JimiDAW.exe"
Name: "{autodesktop}\JimiDAW"; Filename: "{app}\JimiDAW.exe"

[Run]
Filename: "{app}\JimiDAW.exe"; Description: "Launch JimiDAW"; Flags: nowait postinstall skipifsilent
