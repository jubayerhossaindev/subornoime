#define MyAppName "Suborno"
#define MyAppPublisher "Suborno Project"

#ifndef MyArchitecture
#define MyArchitecture "x64"
#endif

#if MyArchitecture = "x64"

#define MyAppId "9FF345F9-29CB-4B45-9EBD-AECDF9008CBB"
#define MyOtherAppId "9960C85A-D398-4777-9140-BC581F29D120"
#define MyOutputName "Suborno-Setup-x64"
#define MyArchitecturesAllowed "x64compatible"
#define MyInstallIn64BitMode "x64compatible"
#define MyMachineRegistryRoot "HKLM64"
#define MyClassesRegistryRoot "HKCR64"

#ifndef MyBuildDir
#define MyBuildDir "..\..\build\windows-msvc"
#endif

#elif MyArchitecture = "x86"

#define MyAppId "9960C85A-D398-4777-9140-BC581F29D120"
#define MyOtherAppId "9FF345F9-29CB-4B45-9EBD-AECDF9008CBB"
#define MyOutputName "Suborno-Setup-x86"
#define MyArchitecturesAllowed "x86compatible and not x64compatible"
#define MyInstallIn64BitMode ""
#define MyMachineRegistryRoot "HKLM32"
#define MyClassesRegistryRoot "HKCR32"

#ifndef MyBuildDir
#define MyBuildDir "..\..\build\windows-msvc-x86"
#endif

#else

#error MyArchitecture must be either "x64" or "x86".

#endif

#include MyBuildDir + "\suborno_version.iss.inc"

[Setup]
AppId={{{#MyAppId}}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\Suborno.exe
OutputDir=Output
OutputBaseFilename={#MyOutputName}
SetupIconFile={#MyBuildDir}\suborno.ico
VersionInfoVersion={#MyAppVersion}
VersionInfoDescription=Suborno Windows Application Setup
VersionInfoProductName=Suborno
VersionInfoCompany={#MyAppPublisher}
LicenseFile=..\..\THIRD_PARTY_NOTICES.md
; Per-machine installation is REQUIRED: the TSF text service is registered
; machine-wide (regsvr32 into HKCR, {commonappdata} shared tree), which only
; an elevated installation can do. User settings stay strictly per-user —
; the application writes only under the running user's RoamingAppData
; (SHGetKnownFolderPath FOLDERID_RoamingAppData), never a shared or
; installer-context location. Inno's "per-user areas used with admin"
; warning refers to the per-user Quick Launch icon below; it affects only
; the installing user's own shortcuts, which is the intended behavior.
PrivilegesRequired=admin
ArchitecturesAllowed={#MyArchitecturesAllowed}

#if MyInstallIn64BitMode != ""
ArchitecturesInstallIn64BitMode={#MyInstallIn64BitMode}
#endif

Compression=lzma
SolidCompression=yes
WizardStyle=modern
Uninstallable=yes
UsePreviousAppDir=no
CloseApplications=yes
AppMutex=Local\Suborno.Desktop.Application

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Files]
Source: "{#MyBuildDir}\Release\Suborno.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\THIRD_PARTY_NOTICES.md"; DestDir: "{app}"; Flags: ignoreversion
; The About dialog opens LICENSE and THIRD_PARTY_NOTICES.md from the
; installation directory, and Documentation points at the README on the web;
; ship the license text and readme so the local documents exist offline.
Source: "..\..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\README.md"; DestDir: "{app}"; Flags: ignoreversion
; Interface language files (data/i18n/<locale>/ui.json). The shell loads the
; locale the settings select ("system" follows the Windows display language);
; a missing file falls back to the compiled English strings, so shipping the
; files only ENABLES the Bangla UI — it can never break the English one.
Source: "..\..\data\i18n\en\ui.json"; DestDir: "{app}\data\i18n\en"; Flags: ignoreversion
Source: "..\..\data\i18n\bn\ui.json"; DestDir: "{app}\data\i18n\bn"; Flags: ignoreversion
Source: "{#MyBuildDir}\Release\suborno_windows_tsf.dll"; DestDir: "{commonappdata}\Suborno\components\tsf\{#MyArchitecture}"; Flags: ignoreversion
; BornoTSF compatibility data: the phonetic autocorrect dictionary and the
; Khipro mim layout specification. The engines load them from the shared
; data tree first (then the per-user override paths, then these bundled
; copies), mirroring BornoTSF's own install layout.
Source: "..\..\assets\dictionaries\ac.json"; DestDir: "{commonappdata}\Suborno\dictionaries"; Flags: ignoreversion
Source: "..\..\assets\layouts\bn-khipro.mim"; DestDir: "{commonappdata}\Suborno\layouts"; Flags: ignoreversion
; Per-layout icons ported from BornoTSF (Borno Phonetic, Avro, Khipro,
; Jatiyo, Probhat plus the application icon set).
Source: "..\..\assets\layout-icons\borno\*.ico"; DestDir: "{commonappdata}\Suborno\assets\layout-icons\borno"; Flags: ignoreversion

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce
Name: "quicklaunchicon"; Description: "Create a &Quick Launch shortcut"; GroupDescription: "Additional icons:"; Flags: checkedonce

[Dirs]
Name: "{commonappdata}\Suborno"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\assets"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\assets\fonts"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\assets\themes"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\assets\layout-icons"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\assets\layout-icons\borno"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\dictionaries"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\layouts"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\components"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\components\tsf"; Permissions: users-readexec
Name: "{commonappdata}\Suborno\components\tsf\{#MyArchitecture}"; Permissions: users-readexec

[InstallDelete]
Type: files; Name: "{app}\suborno_windows_tsf.dll"
Type: files; Name: "{app}\THIRD_PARTY_NOTICES.md"
Type: files; Name: "{app}\LICENSE"
Type: files; Name: "{app}\README.md"
Type: files; Name: "{app}\sqlite3.dll"
Type: files; Name: "{app}\icu*.dll"
Type: files; Name: "{app}\vcruntime*.dll"
Type: files; Name: "{app}\msvcp*.dll"
Type: files; Name: "{app}\concrt*.dll"
Type: files; Name: "{app}\vcomp*.dll"
Type: files; Name: "{app}\ucrtbase.dll"
Type: files; Name: "{app}\api-ms-win-crt-*.dll"

[Icons]
Name: "{group}\Suborno"; Filename: "{app}\Suborno.exe"; IconFilename: "{app}\Suborno.exe"
Name: "{autodesktop}\Suborno"; Filename: "{app}\Suborno.exe"; IconFilename: "{app}\Suborno.exe"; Tasks: desktopicon
Name: "{userappdata}\Microsoft\Internet Explorer\Quick Launch\Suborno"; Filename: "{app}\Suborno.exe"; IconFilename: "{app}\Suborno.exe"; Tasks: quicklaunchicon
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"

[UninstallRun]
Filename: "{sys}\regsvr32.exe"; Parameters: "/s /u ""{commonappdata}\Suborno\components\tsf\{#MyArchitecture}\suborno_windows_tsf.dll"""; Flags: runhidden waituntilterminated; RunOnceId: "UnregisterSubornoTsf"

[UninstallDelete]
Type: dirifempty; Name: "{commonappdata}\Suborno\components\tsf\{#MyArchitecture}"
Type: dirifempty; Name: "{commonappdata}\Suborno\components\tsf"
Type: dirifempty; Name: "{commonappdata}\Suborno\components"

[Run]
; runasoriginaluser: launch the app in the ORIGINAL (usually non-elevated)
; user's context instead of the elevated installer context. The first run
; then creates the settings tree under the actual end user's profile — with
; separate admin credentials this is a DIFFERENT account than the installer's
; — and never inside an administrator's profile.
Filename: "{app}\Suborno.exe"; Description: "Launch Suborno"; Flags: postinstall nowait runasoriginaluser skipifsilent

[Code]

function ExtractUninstallerPath(const CommandLine: String): String;
var
  QuotePosition: Integer;
begin
  Result := Trim(CommandLine);

  if (Length(Result) > 0) and (Result[1] = '"') then begin
    Delete(Result, 1, 1);
    QuotePosition := Pos('"', Result);

    if QuotePosition = 0 then
      Result := ''
    else
      Result := Copy(Result, 1, QuotePosition - 1);

  end else begin
    QuotePosition := Pos(' ', Result);

    if QuotePosition > 0 then
      Result := Copy(Result, 1, QuotePosition - 1);
  end;
end;


function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  PreviousUninstallerKey: String;
  PreviousUninstallCommand: String;
  PreviousUninstaller: String;
  ResultCode: Integer;
begin
  Result := '';

  PreviousUninstallerKey :=
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\{' +
    '{#MyAppId}' +
    '}_is1';

  if not RegKeyExists(
    {#MyMachineRegistryRoot},
    PreviousUninstallerKey
  ) then
    Exit;

  if not RegQueryStringValue(
    {#MyMachineRegistryRoot},
    PreviousUninstallerKey,
    'UninstallString',
    PreviousUninstallCommand
  ) then begin

    Result :=
      'The existing Suborno installation has no registered uninstaller.';
    Exit;
  end;

  PreviousUninstaller :=
    ExtractUninstallerPath(PreviousUninstallCommand);

  if (PreviousUninstaller = '') or
     not FileExists(PreviousUninstaller) then begin

    Result :=
      'The existing Suborno uninstaller could not be found.';
    Exit;
  end;

  if not Exec(
    PreviousUninstaller,
    '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /SP-',
    '',
    SW_HIDE,
    ewWaitUntilTerminated,
    ResultCode
  ) or (ResultCode <> 0) then begin

    Result :=
      'The previous Suborno installation could not be removed for upgrade.';
  end;
end;


function OtherArchitectureInstalled: Boolean;
var
  OtherUninstallKey: String;
begin
  OtherUninstallKey :=
    'Software\Microsoft\Windows\CurrentVersion\Uninstall\{' +
    '{#MyOtherAppId}' +
    '}_is1';

  if '{#MyArchitecture}' = 'x64' then begin

    Result := RegKeyExists(
      HKLM32,
      OtherUninstallKey
    );

  end else begin

    if IsWin64 then
      Result := RegKeyExists(HKLM64, OtherUninstallKey)
    else
      Result := False;
  end;
end;


function DirectoryIsEmpty(const Path: String): Boolean;
var
  FindRec: TFindRec;
  FoundEntry: Boolean;
begin
  Result := False;

  if FindFirst(Path + '\*', FindRec) then begin

    FoundEntry := False;

    try
      repeat

        if (FindRec.Name <> '.') and
           (FindRec.Name <> '..') then begin

          FoundEntry := True;
        end;

      until not FindNext(FindRec);

    finally
      FindClose(FindRec);
    end;

    Result := not FoundEntry;
  end;
end;


procedure RemoveDirectoryIfEmpty(const Path: String);
begin
  if DirExists(Path) and DirectoryIsEmpty(Path) then begin
    RemoveDir(Path);
  end;
end;


function StripSurroundingQuotes(const Value: String): String;
begin
  Result := Trim(Value);

  if (Length(Result) >= 2) and
     (Result[1] = '"') and
     (Result[Length(Result)] = '"') then begin

    Result := Copy(Result, 2, Length(Result) - 2);
  end;
end;


procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
  ExistingDll: String;
  SharedDll: String;
  RegisteredDll: String;
begin

  if CurStep = ssInstall then begin

    ExistingDll :=
      ExpandConstant('{app}\suborno_windows_tsf.dll');

    SharedDll :=
      ExpandConstant(
        '{commonappdata}\Suborno\components\tsf\{#MyArchitecture}\suborno_windows_tsf.dll'
      );

    if RegQueryStringValue(
      {#MyClassesRegistryRoot},
      'CLSID\{E1CC38E7-F986-4371-BA1D-A527CA600677}\InprocServer32',
      '',
      RegisteredDll
    ) then begin

      RegisteredDll :=
        StripSurroundingQuotes(RegisteredDll);

      if CompareText(RegisteredDll, ExistingDll) = 0 then begin

        if not Exec(
          ExpandConstant('{sys}\regsvr32.exe'),
          '/s /u "' + ExistingDll + '"',
          '',
          SW_HIDE,
          ewWaitUntilTerminated,
          ResultCode
        ) or (ResultCode <> 0) then begin

          RaiseException(
            'Unable to unregister the previous Suborno TSF input service.'
          );
        end;

      end else if CompareText(RegisteredDll, SharedDll) = 0 then begin

        if not Exec(
          ExpandConstant('{sys}\regsvr32.exe'),
          '/s /u "' + SharedDll + '"',
          '',
          SW_HIDE,
          ewWaitUntilTerminated,
          ResultCode
        ) or (ResultCode <> 0) then begin

          RaiseException(
            'Unable to unregister the existing Suborno TSF input service.'
          );
        end;
      end;
    end;
  end;


  if CurStep = ssPostInstall then begin

    if not Exec(
      ExpandConstant('{sys}\regsvr32.exe'),
      '/s "' +
      ExpandConstant(
        '{commonappdata}\Suborno\components\tsf\{#MyArchitecture}\suborno_windows_tsf.dll'
      ) +
      '"',
      '',
      SW_HIDE,
      ewWaitUntilTerminated,
      ResultCode
    ) or (ResultCode <> 0) then begin

      RaiseException(
        'Unable to register the Suborno TSF input service.'
      );
    end;
  end;
end;


procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  SharedDirectory: String;
begin

  if CurUninstallStep = usPostUninstall then begin

    if not OtherArchitectureInstalled then begin

      if RegValueExists(
        HKEY_CURRENT_USER,
        'Software\Microsoft\Windows\CurrentVersion\Run',
        'Suborno'
      ) and not RegDeleteValue(
        HKEY_CURRENT_USER,
        'Software\Microsoft\Windows\CurrentVersion\Run',
        'Suborno'
      ) then begin

        RaiseException(
          'Unable to remove the Suborno sign-in launch preference.'
        );
      end;

      SharedDirectory :=
        ExpandConstant('{commonappdata}\Suborno');

      RemoveDirectoryIfEmpty(SharedDirectory + '\layouts');
      RemoveDirectoryIfEmpty(SharedDirectory + '\dictionaries');
      RemoveDirectoryIfEmpty(SharedDirectory + '\assets\fonts');
      RemoveDirectoryIfEmpty(SharedDirectory + '\assets\themes');
      RemoveDirectoryIfEmpty(SharedDirectory + '\assets');
      RemoveDirectoryIfEmpty(SharedDirectory + '\components');
      RemoveDirectoryIfEmpty(SharedDirectory);
    end;
  end;
end;
