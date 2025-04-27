Name:           piper
Version:        1.0.0
Release:        1%{?dist}
Summary:        PipeWire and ALSA volume synchronizer daemon

License:        MIT
URL:            https://github.com/FakeApate/piper
Source0:        piper-%{version}.tar.gz

BuildRequires:  gcc, cmake, make, pipewire-devel, alsa-lib-devel
Requires:       pipewire, alsa-lib

%description
Piper is a daemon that synchronizes ALSA hardware mixer volume and PipeWire node volume
automatically.

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%post
systemctl daemon-reexec
systemctl enable --now piper.service

%files
/usr/bin/piper
/usr/lib/systemd/system/piper.service

%changelog
* Sun Apr 27 2025 Samuel Imboden <imboden.samuel@protonmail.ch> - 1.0-1
- Initial RPM package
