Name:       avbot
Version:    4.0
Release:    1%{?dist}
Summary:    The qq/irc/xmpp protocol bridge bot by avplayer.org
License:    GPLv2+
Group:      Network/Utilites
URL: http://github.com/avplayer/avbot
Source0:    http://github.com/avplayer/avbot/files/avbot-%{version}.tar.bz2

BuildRoot:  %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

BuildRequires: cmake

Requires: openssl zlib libgcc

%description
The qq/irc/xmpp protocol bridge bot by avplayer.org

%prep
%setup -q

%build
export CXXFLAGS="-Os "
export CFLAGS="-Os  "
export LDFLAGS=" -Wl,-O1 "
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} .

%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=${RPM_BUILD_ROOT} NO_INDEX=true install

%clean
rm -rf $RPM_BUILD_ROOT

%files
/

%changelog
* Fri Mar 8 2013 microcai 
- The first version.
