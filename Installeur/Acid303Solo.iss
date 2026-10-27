#define MyAppName "Acid303 Solo"
#define MyAppVersion "3.0.0"

[Setup]
AppId=Acid303SoloVST3
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} v{#MyAppVersion}
AppPublisher=Burnout Theory
AppPublisherURL=https://github.com/burnouttheory
AppSupportURL=https://github.com/burnouttheory
DefaultDirName={commoncf64}\VST3
UsePreviousAppDir=yes
DisableDirPage=no
DisableProgramGroupPage=yes
OutputDir=Output
OutputBaseFilename=Acid303Solo_Setup_{#MyAppVersion}_OFFICIEL
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallFilesDir={commonappdata}\Acid303Solo\Uninstall
CloseApplications=yes
RestartApplications=no
WizardStyle=modern

; IMPORTANT: activer uniquement avec CERTIFICAT DE SIGNATURE RECONNU.
; Le compilateur Inno signe alors Setup, Uninstall et les copies temporaires.
; Cette definition est passee par CREER_INSTALLATEUR_SIGNE.bat.
#ifdef Acid303Signed
SignTool=acid303
SignedUninstaller=yes
#endif

[Languages]
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
french.CloseAudioApps=Avant de poursuivre, sauvegardez vos projets et fermez FL Studio ainsi que tout autre logiciel utilisant Acid303 Solo. Vos dossiers personnels (presets, skins et MIDI) seront préservés.
english.CloseAudioApps=Before proceeding, save your projects and close FL Studio and any other audio application using Acid303 Solo. Your personal presets, skins and MIDI files will be preserved.

[Files]
; JUCE Windows VST3 est un dossier (bundle) : il faut conserver Contents\x86_64-win\...
Source: "payload\Acid303 Solo.vst3\*"; DestDir: "{app}\Acid303 Solo.vst3"; Flags: recursesubdirs createallsubdirs ignoreversion
; Only files placed in FactoryContent are distributed.  NO automatic access to the author's Documents folder.
; A user-level JUCE startup routine copies missing factory files into the real Documents folder,
; avoiding both elevated-install wrong-user paths and overwrites of customised content.
Source: "..\FactoryContent\skins\*"; DestDir: "{commonappdata}\Acid303Solo\FactoryContent\skins"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\FactoryContent\presets\*"; DestDir: "{commonappdata}\Acid303Solo\FactoryContent\presets"; Flags: recursesubdirs createallsubdirs ignoreversion
Source: "..\FactoryContent\midi\*"; DestDir: "{commonappdata}\Acid303Solo\FactoryContent\midi"; Flags: recursesubdirs createallsubdirs ignoreversion
; PDF user manuals (French/English) installed alongside the factory resources.
Source: "..\Manuals\*.pdf"; DestDir: "{commonappdata}\Acid303Solo\Manuals"; Flags: ignoreversion
; Legal notices shipped WITH the installed plugin (not only in source archive).
Source: "..\LICENSES\*"; DestDir: "{commonappdata}\Acid303Solo\Licenses"; Flags: recursesubdirs createallsubdirs ignoreversion

[Code]
function InitializeSetup(): Boolean;
begin
  Result := (MsgBox(ExpandConstant('{cm:CloseAudioApps}'), mbConfirmation, MB_OKCANCEL) = IDOK);
end;
