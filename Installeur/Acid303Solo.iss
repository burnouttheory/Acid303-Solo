#define MyAppName "Acid303 Solo"
#define MyAppVersion "0.30.2"

[Setup]
AppId=Acid303SoloVST3
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher=Acid303 Independent
DefaultDirName={commoncf64}\VST3
UsePreviousAppDir=yes
DisableDirPage=no
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=Acid303Solo_Setup_{#MyAppVersion}
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallFilesDir={commonappdata}\Acid303Solo\Uninstall
CloseApplications=yes
RestartApplications=no
WizardStyle=modern

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"

[Files]
; JUCE Windows VST3 est un dossier (bundle) : il faut conserver Contents\x86_64-win\...
Source: "payload\Acid303 Solo.vst3\*"; DestDir: "{app}\Acid303 Solo.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion

[Code]
function InitializeSetup(): Boolean;
begin
  Result := (MsgBox('Avant de poursuivre, sauvegardez vos projets et fermez FL Studio ainsi que tout autre logiciel audio utilisant Acid303 Solo. L''installateur ne modifie pas vos dossiers personnels Documents\Acid303\midi, presets et skins.', mbConfirmation, MB_OKCANCEL) = IDOK);
end;
