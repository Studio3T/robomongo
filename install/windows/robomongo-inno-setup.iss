; Main params for Robomongo setup 
; BASE_DIR and VERSION defines folder, using this schema: {#BASE_DIR}robomongo-win-i386-{#VERSION}
; Also, already defined constant {#SourcePath} points to this *.iss file.
#define VERSION "0.6.6"
#define BASE_DIR "D:\Apps\robomongo\"
#define OUTPUT_DIR "D:\Apps\robomongo\installers"

; Additional params (depends on the upper ones)
#define FULL_VERSION VERSION + " beta"



[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{E6583054-DBD6-4EDE-BA79-7492214E9A01}
AppName=Robomongo
AppVersion={#FULL_VERSION}
;AppVerName=Robomongo 0.6.5 beta
AppPublisher=Paralect
AppPublisherURL=http://www.robomongo.org/
AppSupportURL=http://www.robomongo.org/
AppUpdatesURL=http://www.robomongo.org/
DefaultDirName={pf}\Robomongo
DefaultGroupName=Robomongo
DisableProgramGroupPage=yes
OutputDir={#OUTPUT_DIR}
OutputBaseFilename=Robomongo-{#VERSION}
Compression=lzma
SolidCompression=yes
WizardImageFile="{#SourcePath}\database.bmp"
WizardSmallImageFile="{#SourcePath}\database-small.bmp"
;SetupIconFile="{#SourcePath}\robomongo.ico"

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; 

[Files]
Source: "{#BASE_DIR}robomongo-win-i386-{#VERSION}\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\Robomongo"; Filename: "{app}\robomongo.exe"
Name: "{commondesktop}\Robomongo"; Filename: "{app}\robomongo.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\robomongo.exe"; Description: "{cm:LaunchProgram,Robomongo}"; Flags: nowait postinstall skipifsilent
