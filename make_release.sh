#!/usr/bin/env bash
# create_release.sh - Generate release distributables for Xen
# NOT INTENDED FOR EXTERNAL USE

if [ $# -ne 1 ]; then
    echo "No version was provided. Exiting."
    exit 1
fi

if [ -z "$CODE_DIR" ]; then
    echo "CODE_DIR is not set. Exiting."
    exit 1
fi

VERSION=$1
STARTUP_DIR=$(pwd)
START_TIME=$(date +%s)
FILE_VERSION=$(rg -oPN 'VERSION_STRING "\K[0-9]+\.[0-9]+\.[0-9]+' src/xen/xversion.h)

# Ensure version number in src/xen/version.h matches the provided version number
if [ "$FILE_VERSION" = "$VERSION" ]; then
    echo "✓ Version matches: $FILE_VERSION"
else
    echo "✗ Version mismatch!"
    echo "  File version: $FILE_VERSION"
    echo "  Expected: $VERSION"
    exit 1
fi

echo "Creating distributables for Xen v$VERSION"

make clean-all
make all-platforms

REL_DIR=$CODE_DIR/Xen/releases/$VERSION

if [ -e "$REL_DIR" ]; then
    rm -rf $REL_DIR
fi

mkdir -p "$REL_DIR/linux"
mkdir -p "$REL_DIR/windows"
mkdir -p "$REL_DIR/macos"

create_spec_files() {
    mkdir -p spec

    # Debian CONTROL spec
    cat >spec/xen.control <<EOF
package: xen
version: ${VERSION} 
section: devel
priority: optional
architecture: amd64
maintainer: Jake Rieger <contact.jakerieger@gmail.com>
description: xen
 Xen programming language interpreter and tools.
EOF

    # RPM spec
    cat >spec/xen.spec <<EOF
Name:           Xen
%global debug_package %{nil}
Version:        ${VERSION}
Release:        1%{?dist}
Summary:        Xen

License:        ISC
URL:            https://jakerieger.github.io/Xen
Source0:        Xen-%{version}.tar.gz

BuildRequires:  gcc
BuildRequires:  bash
BuildRequires:  make

%description
Xen programming language interpreter and tools.

%prep
%setup -q -n Xen-%{version}

%build
make BUILD_TYPE=release TARGET_PLATFORM=linux

%install
rm -rf \$RPM_BUILD_ROOT

# Install the main binary
install -D -m 0755 build/linux-release/bin/xen \$RPM_BUILD_ROOT%{_bindir}/xen

# Install examples to /usr/share/xen
mkdir -p \$RPM_BUILD_ROOT%{_datadir}/xen
cp -r examples \$RPM_BUILD_ROOT%{_datadir}/xen/

%files
%license LICENSE
%doc README.md
%{_bindir}/xen
%{_datadir}/xen/examples/*

%changelog
* Fri Jan 09 2026 Jake Rieger <contact.jakerieger@gmail.com> - ${VERSION}-1
- See https://github.com/XenLanguage/Xen/blob/master/CHANGELOG.md
EOF

    # Inno Setup Spec
    cat >spec/xen.iss <<EOF
[Setup]
AppName=Xen
AppVersion=${VERSION}
DefaultDirName={commonpf}\Xen
DefaultGroupName=Xen
OutputDir=.
OutputBaseFilename=Xen-${VERSION}-windows-x64
Compression=lzma2
SolidCompression=yes

[Files]
Source: "..\build\windows-release\bin\xen.exe"; DestDir: "{app}"; Flags: ignoreversion
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
EOF
}

make_linux_rpm() {
    RPMBUILD_ROOT="$HOME/rpmbuild"
    LINUX_RPM_TAR="$RPMBUILD_ROOT/SOURCES/Xen-$VERSION.tar.gz"

    tar czf $LINUX_RPM_TAR \
        --transform "s,^,Xen-${VERSION}/," \
        --exclude='.git*' \
        --exclude='build*' \
        --exclude='*.tar.gz' \
        src/ examples/ Makefile \
        LICENSE README.md

    rpmbuild -ba spec/xen.spec

    OUT="$REL_DIR/linux/Xen-$VERSION-linux-x64.rpm"
    find "$RPMBUILD_ROOT/RPMS/x86_64" -name "Xen-$VERSION-*.rpm" ! -name "*debug*" -exec cp {} $OUT \;

    gpg --detach-sign --armor --output "$OUT.sig" $OUT
}

make_linux_deb() {
    DEB_ROOT="$REL_DIR/linux/Xen-$VERSION-linux-x64"
    mkdir -p "$DEB_ROOT/DEBIAN"
    mkdir -p "$DEB_ROOT/usr/bin"
    mkdir -p "$DEB_ROOT/usr/share/xen/examples"
    mkdir -p "$DEB_ROOT/usr/share/doc/xen"

    cp build/linux-release/bin/xen "$DEB_ROOT/usr/bin/"
    cp -r examples "$DEB_ROOT/usr/share/xen/"
    cp README.md LICENSE "$DEB_ROOT/usr/share/doc/xen/"
    cp spec/xen.control "$DEB_ROOT/DEBIAN/control"

    dpkg-deb --build $DEB_ROOT
    rm -rf $DEB_ROOT

    OUT="$REL_DIR/linux/Xen-$VERSION-linux-x64.deb"
    gpg --detach-sign --armor --output "$OUT.sig" $OUT
}

make_linux_tarball() {
    LINUX_BIN_TAR="$REL_DIR/linux/Xen-$VERSION-linux-x64.tar.gz"
    mkdir bin
    cp build/linux-release/bin/xen bin/
    tar -czf $LINUX_BIN_TAR bin/ examples/ README.md LICENSE
    rm -rf bin

    gpg --detach-sign --armor --output "$LINUX_BIN_TAR.sig" $LINUX_BIN_TAR
}

make_windows_zip() {
    WINDOWS_BIN_ZIP="$REL_DIR/windows/Xen-$VERSION-windows-x64.zip"
    mkdir bin
    cp build/windows-release/bin/xen.exe bin/
    zip -r $WINDOWS_BIN_ZIP bin/ README.md LICENSE
    rm -rf bin

    gpg --detach-sign --armor --output "$WINDOWS_BIN_ZIP.sig" $WINDOWS_BIN_ZIP
}

make_windows_exe() {
    FILENAME="Xen-$VERSION-windows-x64.exe"
    WINDOWS_EXE="$REL_DIR/windows/$FILENAME"
    (cd spec && wine64 $HOME/.wine/drive_c/Program\ Files\ \(x86\)/Inno\ Setup\ 6/ISCC.exe xen.iss)
    mv spec/$FILENAME "$REL_DIR/windows/"

    gpg --detach-sign --armor --output "$WINDOWS_EXE.sig" $WINDOWS_EXE
}

make_source_tarball() {
    SOURCE_TAR="$REL_DIR/Xen-$VERSION.tar.gz"
    tar -czf $SOURCE_TAR src/ examples/ .clang-format README.md LICENSE Makefile

    gpg --detach-sign --armor --output "$SOURCE_TAR.sig" $SOURCE_TAR
}

make_macos_tarball() {
    MACOS_BIN_TAR="$REL_DIR/macos/Xen-$VERSION-macos-x64.tar.gz"
    mkdir bin
    cp build/macos-release/bin/xen bin/
    tar -czf $MACOS_BIN_TAR bin/ examples/ README.md LICENSE
    rm -rf bin

    gpg --detach-sign --armor --output "$MACOS_BIN_TAR.sig" $MACOS_BIN_TAR
}

make_macos_arm_tarball() {
    MACOS_BIN_TAR="$REL_DIR/macos/Xen-$VERSION-macos-arm64.tar.gz"
    mkdir bin
    cp build/macos-arm-release/bin/xen bin/
    tar -czf $MACOS_BIN_TAR bin/ examples/ README.md LICENSE
    rm -rf bin

    gpg --detach-sign --armor --output "$MACOS_BIN_TAR.sig" $MACOS_BIN_TAR
}

# Generate spec files for DEB, RPM, and Inno
create_spec_files
# Linux
make_linux_rpm
make_linux_deb
make_linux_tarball
# Windows
make_windows_zip
make_windows_exe
# macOS
make_macos_tarball
make_macos_arm_tarball
# Source Code
make_source_tarball

# Cleanup specs
rm -rf spec

# Copy verification key
cp $HOME/.gnupg/public-key.asc "$REL_DIR/public_key.asc"

END_TIME=$(date +%s)

echo ""
echo "=== DONE ==="
echo "Created Xen v$VERSION"
echo "Took $(echo "$END_TIME - $START_TIME" | bc) seconds"
