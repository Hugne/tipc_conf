#
# spec file for package client
#
# Copyright (c) 2015 SUSE LINUX Products GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#

Name:           tipcconf
Summary:        Autoconfig client for TIPC
Version:        0.1.0
Release:        1%{?dist}
License:        BSD 2-clause
Group:          Applications/System
URL:            http://tipc.sourceforge.net
BuildRequires:  autoconf, automake
BuildRequires:	libavahi-devel
BuildRequires:	libmnl-devel
BuildRoot:      %{_tmppath}/%{name}-%{version}-build
Source:		tipcconf-0.1.0.tar.gz

%define _unpackaged_files_terminate_build 0

%description
tipcconf is an autoconfig client for TIPC. When invoked, it
will try to resolve TipcConfigService through mDNS-SD and
request configuration data from there. The data is then
parsed, and the configuration is passed down through netlink
to the TIPC kernel module.

%prep
%setup -n tipcconf-0.1.0
#%setup -q

%build
autoreconf -if
%configure LIBJANSSON_LIBS="-Wl,-Bstatic -ljansson" LIBMNL_LIBS="-Wl,-Bdynamic -lmnl" LIBAVAHICORE_LIBS="-Wl,-Bdynamic -lavahi-core -lavahi-common"
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot} %{?_smp_mflags}

%post

%postun

%files

/usr/bin/client
%defattr(-,root,root)

%changelog
* Thu Jun 11 2015 Erik Hugne <erik.hugne@ericsson.com>
- Initial version

