Name:           Xen
Version:        0.5.1
Release:        1%{?dist}
Summary:        Xen

License:        ISC
URL:            https://jakerieger.github.io/Xen
Source0:        Xen-%{version}.tar.gz

BuildRequires:  gcc, make

%description
Xen programming language interpreter and tools.

%prep
%setup -q -n Xen-%{version}
# Remove Windows-specific files
rm -rf bin_win/

%build
make %{?_smp_mflags}

%install
rm -rf $RPM_BUILD_ROOT
# Install the main binary
make install DESTDIR=$RPM_BUILD_ROOT PREFIX=/usr

# Install examples to /usr/share/xen
mkdir -p $RPM_BUILD_ROOT%{_datadir}/xen
cp -r examples $RPM_BUILD_ROOT%{_datadir}/xen/

%files
%license LICENSE
%doc README.md
%{_bindir}/xen
%{_datadir}/xen/examples/*

%changelog
* Mon Jan 07 2026 Jake Rieger <contact.jakerieger@gmail.com> - 0.5.1-1
- bug fixes and improvements
