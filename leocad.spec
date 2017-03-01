%define qt5 0

%if 0%{?suse_version}
%define dist .openSUSE%(echo %{suse_version} | sed 's/0$//')
%endif

%if 0%{?sles_version}
%define dist .SUSE%(echo %{sles_version} | sed 's/0$//')
%endif

%if %(if [[ "%{vendor}" == obs://* ]] ; then echo 1 ; else echo 0 ; fi)
%define opensuse_bs 1
%endif

%if 0%{?centos_ver}
%define centos_version %{centos_ver}00
%endif

Summary: CAD program for creating virtual LEGO models
Name: leocad
URL: http://leocad.org
%if 0%{?suse_version} || 0%{?sles_version}
Group: Productivity/Graphics/Viewers
%endif
%if 0%{?mdkversion} || 0%{?rhel_version} 
Group: Graphics
%endif
%if 0%{?fedora} || 0%{?centos_version}
Group: Amusements/Graphics
%endif
Version: 0.83.2
Release: 1%{?dist}
%if 0%{?mdkversion} || 0%{?rhel_version} || 0%{?fedora} || 0%{?centos_version} || 0%{?scientificlinux_version} || 0%{?mageia}
License: GPLv2+
%endif
%if 0%{?suse_version} || 0%{?sles_version}
License: GPL-2.0+
BuildRequires: fdupes
%endif
Packager: Peter Bartfai <pbartfai@stardust.hu>
BuildRoot: %{_builddir}/%{name}

%if 0%{?fedora} || 0%{?rhel_version} || 0%{?centos_version} || 0%{?scientificlinux_version}
%if ( 0%{?centos_version}>=600 || 0%{?rhel_version}>=600 || 0%{?scientificlinux_version}>=600 )
%if 0%{?qt5}
BuildRequires: qt5-qtbase-devel >= 5.4.0, qt5-linguist
%else
BuildRequires: qt-devel >= 1:4.7.0
%endif
%endif
%endif
%if 0%{?fedora}
%if 0%{?qt5}
BuildRequires: qt5-qtbase-devel >= 5.4.0, qt5-linguist
%else
BuildRequires: qt-devel
%endif
%endif
%if 0%{?opensuse_bs}!=1
BuildRequires: git
%endif
%if (0%{?rhel_version}<700 && 0%{?centos_version}<700 && 0%{?scientificlinux_version}<600)
%else
BuildRequires: libjpeg-turbo-devel
%endif
BuildRequires: gcc-c++, libpng-devel, make


%if 0%{?fedora}
BuildRequires: libjpeg-turbo-devel
%if 0%{?opensuse_bs}
BuildRequires: samba4-libs
%if 0%{?fedora_version}==22
BuildRequires: qca
%endif
%if 0%{?fedora_version}==23
BuildRequires: qca, gnu-free-sans-fonts
%endif
%endif
%endif

%if 0%{?suse_version}
BuildRequires: update-desktop-files
%if 0%{?qt5}
BuildRequires: libqt5-qtbase-devel >= 5.4.0, zlib-devel, libqt5-linguist
%else
BuildRequires: libqt4-devel
%endif
Requires(pre): gconf2
%if 0%{?suse_version} > 1220
BuildRequires: glu-devel
%endif
%if 0%{?opensuse_bs}
BuildRequires:	-post-build-checks
%endif
%endif

%if 0%{?sles_version}
%if 0%{?opensuse_bs}
BuildRequires:	-post-build-checks
%endif
Requires(post): desktop-file-utils
%endif

%if 0%{?mageia}
BuildRequires: libqt4-devel
%if 0%{?opensuse_bs}
%ifarch x86_64
BuildRequires: lib64uuid-devel
%else
BuildRequires: libuuid-devel
%endif
%endif
%endif

%if 0%{?mdkversion}
BuildRequires: libqt4-devel
# For openSUSE Build Service
%if 0%{?opensuse_bs}
%if (0%{?mdkversion} != 200910) && (0%{?mdkversion} != 201000)
BuildRequires: kde-l10n-en_GB
%endif
BuildRequires: aspell-en, myspell-en_US
%endif
%endif

%if ( 0%{?centos_version}<600 && 0%{?centos_version}>=500 ) || ( 0%{?rhel_version}<600 && 0%{?rhel_version}>=500 )
BuildRequires: qt4-devel >= 1:4.7.0
%endif

%description
CAD program for creating virtual LEGO models.
It has an intuitive interface, designed to allow 
new users to start creating new models without 
having to spend too much time learning the 
application.

%prep
cd $RPM_SOURCE_DIR
if [ -s leocad.tar.gz ] ; then
	if [ -d leocad ] ; then rm -rf leocad ; fi
	tar zxf leocad.tar.gz
else
	if [ -f leocad-git.tar.gz ] ; then
		if [ -d leocad ] ; then rm -rf leocad ; fi
		mkdir leocad
		cd leocad
		tar zxf ../leocad-git.tar.gz --strip=1
	elif [ -d leocad ] ; then
		cd leocad
		git pull
		cd ..
	else
		git clone https://github.com/leozide/leocad
	fi
fi

%build
cd $RPM_SOURCE_DIR/leocad
%ifarch i386 i486 i586 i686
%define qplatform linux-g++-32
%endif
%ifarch x86_64
%define qplatform linux-g++-64
%endif
%if ( 0%{?centos_version}<600 && 0%{?centos_version}>=500 ) || ( 0%{?rhel_version}<600 && 0%{?rhel_version}>=500 )
if [ -x %{_libdir}/qt4/bin/qmake ] ; then
export PATH=%{_libdir}/qt4/bin:$PATH
fi
%endif
%if (0%{?qt5}!=1)
%ifarch x86_64
export RPM_OPT_FLAGS="$RPM_OPT_FLAGS -I%{_libdir}/qt4/include"
%endif
%endif
%if 0%{?fedora}==23
%ifarch x86_64
export RPM_OPT_FLAGS="$RPM_OPT_FLAGS -fPIC"
export Q_CXXFLAGS="$Q_CXXFLAGS -fPIC"
%endif
%endif
%if 0%{?qt5}
export RPM_OPT_FLAGS="$RPM_OPT_FLAGS -fPIC"
if which qmake-qt5 >/dev/null 2>/dev/null ; then
        qmake-qt5 -spec %{qplatform} DISABLE_UPDATE_CHECK=1 LDRAW_LIBRARY_PATH=/usr/share/ldraw QMAKE_CXXFLAGS+="$Q_CXXFLAGS"
else
        qmake -spec %{qplatform} DISABLE_UPDATE_CHECK=1 LDRAW_LIBRARY_PATH=/usr/share/ldraw QMAKE_CXXFLAGS+="$Q_CXXFLAGS"
fi
%else
if which qmake-qt4 >/dev/null 2>/dev/null ; then
	qmake-qt4 -spec %{qplatform} DISABLE_UPDATE_CHECK=1 LDRAW_LIBRARY_PATH=/usr/share/ldraw QMAKE_CXXFLAGS+="$Q_CXXFLAGS"
else
	qmake -spec %{qplatform} DISABLE_UPDATE_CHECK=1 LDRAW_LIBRARY_PATH=/usr/share/ldraw QMAKE_CXXFLAGS+="$Q_CXXFLAGS"
fi
%endif
make clean
make TESTING="$RPM_OPT_FLAGS"
%if 0%{?qt5}
if which lrelease-qt5 >/dev/null 2>/dev/null ; then
        lrelease-qt5 leocad.pro
else
        lrelease leocad.pro
fi
%else
if which lrelease-qt4 >/dev/null 2>/dev/null ; then
	lrelease-qt4 leocad.pro
else
	lrelease leocad.pro
fi
%endif
%if 0%{?qt5} != 1
%endif

%install
cd $RPM_SOURCE_DIR/leocad
install -d $RPM_BUILD_ROOT%{_bindir}
install -d $RPM_BUILD_ROOT%{_datadir}/leocad
install -d $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/mimetypes
install -d $RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps
install -d $RPM_BUILD_ROOT%{_mandir}/man1
install -m 755 build/release/leocad $RPM_BUILD_ROOT%{_bindir}/leocad
install -m 644 docs/README.txt $RPM_BUILD_ROOT%{_datadir}/leocad/README.txt
install -m 644 docs/CREDITS.txt $RPM_BUILD_ROOT%{_datadir}/leocad/CREDITS.txt
install -m 644 docs/COPYING.txt $RPM_BUILD_ROOT%{_datadir}/leocad/COPYING.txt
install -m 644 docs/leocad.1 $RPM_BUILD_ROOT%{_mandir}/man1/leocad.1
gzip -f $RPM_BUILD_ROOT%{_mandir}/man1/leocad.1
mkdir -p $RPM_BUILD_ROOT%{_datadir}/mime/packages/
mkdir -p $RPM_BUILD_ROOT%{_datadir}/applications/
install -m 644 qt/leocad.xml  \
				$RPM_BUILD_ROOT%{_datadir}/mime/packages/leocad.xml
install -m 644 qt/leocad.desktop \
				$RPM_BUILD_ROOT%{_datadir}/applications/leocad.desktop
install -m 644 resources/application-vnd.leocad.svg \
				$RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/mimetypes/application-vnd.leocad.svg
install -m 644 resources/leocad.svg \
				$RPM_BUILD_ROOT%{_datadir}/icons/hicolor/scalable/apps/leocad.svg
%if 0%{?suse_version}
%suse_update_desktop_file leocad Graphics
%endif

%files
%if 0%{?sles_version} || 0%{?suse_version}
%defattr(-,root,root)
%endif
%{_bindir}/leocad
%dir %{_datadir}/leocad
%doc %{_datadir}/leocad/README.txt
%doc %{_datadir}/leocad/CREDITS.txt
%doc %{_datadir}/leocad/COPYING.txt
%{_datadir}/mime/packages/leocad.xml
%{_datadir}/applications/leocad.desktop
%{_datadir}/icons/hicolor/scalable/mimetypes/application-vnd.leocad.svg
%{_datadir}/icons/hicolor/scalable/apps/leocad.svg
%if 0%{?mdkversion} || 0%{?mageia}
%{_mandir}/man1/leocad.1.xz
%else
%{_mandir}/man1/leocad.1.gz
%endif

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
* Tue Sep 16 2016 - pbartfai (at) stardust.hu 0.90
- Initial version

