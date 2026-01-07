[Setup]
AppName=Xen
AppVersion=0.5.1
DefaultDirName={commonpf}\Xen
DefaultGroupName=Xen
OutputDir=.
OutputBaseFilename=Xen-0.5.1-windows-x64
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\bin_win\xen.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\examples\*"; DestDir: "{app}\examples"; Flags: ignoreversion recursesubdirs
Source: "..\README.md"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\Xen"; Filename: "{app}\xen.exe"

[Tasks]
Name: "envpath"; Description: "Add to PATH"; GroupDescription: "Additional tasks:"

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
var
    ResultCode: Integer;
begin
    if CurStep = ssPostInstall then
    begin
        if WizardIsTaskSelected('envpath') then
        begin
            Exec('setx', 'PATH "%PATH%;' + ExpandConstant('{app}') + '"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
        end;
    end;
end;
