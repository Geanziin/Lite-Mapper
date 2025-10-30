; Inno Setup script for KeyMapper C++ build
#define MyAppName "KeyMapper"
#ifndef MyAppVersion
#define MyAppVersion "6.0.0"
#endif
#define MyAppPublisher "GEANSWAG"
#define MyAppExeName "keymapper_app.exe"

[Setup]
AppId={{7E8C0E5C-4C65-4C18-9A6F-2B5E7F3B0A4F}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={pf64}\KeyMapper
DisableDirPage=no
DisableProgramGroupPage=yes
OutputDir=.
OutputBaseFilename=KeyMapperCpp-Setup
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog

[Languages]
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"

[Tasks]
Name: "desktopicon"; Description: "Criar atalho na área de trabalho"; GroupDescription: "Tarefas adicionais:"; Flags: unchecked

[Files]
; Instala tudo que foi preparado no passo de empacotamento (cpp-dist)
Source: "cpp-dist\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{autoprograms}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\imagens\logo.ico"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon; IconFilename: "{app}\imagens\logo.ico"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Executar {#MyAppName}"; Flags: nowait postinstall skipifsilent runascurrentuser
