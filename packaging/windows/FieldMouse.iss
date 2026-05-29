#ifndef MyAppName
  #define MyAppName "Field Mouse"
#endif
#ifndef AppVersion
  #define AppVersion "1.0.0"
#endif
#ifndef AppVersionTag
  #define AppVersionTag "v1.0.0"
#endif
#ifndef AppPublisher
  #define AppPublisher "Field Mouse"
#endif
#ifndef AppId
  #define AppId "{{B762D3E2-211B-4B6C-B1DB-2A67A86933F3}}"
#endif
#ifndef SourceDir
  #define SourceDir "."
#endif
#ifndef OutputDir
  #define OutputDir "."
#endif
#ifndef OutputBaseFilename
  #define OutputBaseFilename "FieldMouse-setup-v1.0.0"
#endif
#ifndef SetupIconFile
  #define SetupIconFile "..\\..\\assets\\icons\\field-mouse.ico"
#endif
#ifndef LicenseFile
  #define LicenseFile "..\\..\\LICENSE"
#endif

[Setup]
AppId={#AppId}
AppName={#MyAppName}
AppVersion={#AppVersion}
AppVerName={#MyAppName} {#AppVersionTag}
AppPublisher={#AppPublisher}
DefaultDirName={autopf64}\Field Mouse
DefaultGroupName=Field Mouse
UninstallDisplayIcon={app}\FieldMouse.exe
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
ChangesAssociations=no
Compression=lzma2/max
SolidCompression=yes
DisableProgramGroupPage=yes
LicenseFile={#LicenseFile}
OutputDir={#OutputDir}
OutputBaseFilename={#OutputBaseFilename}
PrivilegesRequired=admin
SetupIconFile={#SetupIconFile}
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\Field Mouse"; Filename: "{app}\FieldMouse.exe"
Name: "{autodesktop}\Field Mouse"; Filename: "{app}\FieldMouse.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; Flags: unchecked

[Run]
Filename: "{app}\FieldMouse.exe"; Description: "Launch Field Mouse"; Flags: nowait postinstall skipifsilent
