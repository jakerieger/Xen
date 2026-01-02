!include "MUI2.nsh"

!ifndef VERSION
    !define VERSION "0.0.0"
!endif

Name "Xen"
OutFile "../releases/${VERSION}/windows/Xen-${VERSION}-windows-x64.exe"
InstallDir "$PROGRAMFILES\Xen"

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

Section "Install"
    SetOutPath "$INSTDIR"
    File "../README.md"
    File "../LICENSE"
    SetOutPath "$INSTDIR/bin"
    File "../bin_win/xen.exe"
    SetOutPath "$INSTDIR/examples"
    File "../examples/"
SectionEnd
