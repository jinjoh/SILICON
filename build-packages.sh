#!/bin/sh

PACKAGE=degate
VERSION=0.0.5
ARCH=`dpkg-architecture -qDEB_BUILD_ARCH`

make clean
make DEBUG_FLAGS="" OPTIMIZATION_FLAGS="-O3 -finline-functions -finline-functions-called-once -fearly-inlining"
echo compiling done

strip degate
strip plugins/*.so

enroll_src_package() {
    PACKAGE=$1
    VERSION=$2
    ARCH=src
    ROOT=$PACKAGE-$VERSION-$ARCH

    if [ -d $ROOT ] ; then
	echo cleaning previous tree
	rm -rf $ROOT
    fi

    echo creating a tree for the package
    mkdir $ROOT

    mkdir $ROOT/glade
    cp glade/*.glade $ROOT/glade
    mkdir $ROOT/icons
    cp icons/*.png $ROOT/icons
    mkdir $ROOT/gui
    cp gui/*.cc gui/*.h $ROOT/gui
    mkdir $ROOT/lib
    cp lib/*.[ch] $ROOT/lib
    mkdir -p $ROOT/logiclayerserialization/src
    cp logiclayerserialization/protocol.asn1 \
	logiclayerserialization/Makefile \
	$ROOT/logiclayerserialization
    cp logiclayerserialization/src/*.[ch] \
	$ROOT/logiclayerserialization/src
    mkdir $ROOT/plugins
    cp plugins/*.cc plugins/*.h $ROOT/plugins
    cp Doxyfile Makefile LICENCE.TXT changelog Bugs.txt build-packages.sh $ROOT
    mkdir $ROOT/doc
    mkdir $ROOT/doc/man
    cp doc/man/degate.1 $ROOT/doc/man 
    mkdir $ROOT/doc/manual
    cp doc/manual/degate_doku.pdf $ROOT/doc/manual
    cp FreeSans.ttf $ROOT/

    tar cvzf $ROOT.tgz $ROOT
}

enroll_deb_package() {
    PACKAGE=$1
    VERSION=$2
    ARCH=$3

    ROOT=$PACKAGE-$VERSION-$ARCH

    if [ -d $ROOT ] ; then
	echo cleaning previous tree
	rm -rf $ROOT
    fi

    echo creating a tree for the package
    mkdir $ROOT
    mkdir -p $ROOT/usr/share/degate
    mkdir -p $ROOT/usr/bin
    mkdir -p $ROOT/usr/share/man/man1
    mkdir -p $ROOT/usr/share/doc/degate/
    mkdir -p $ROOT/usr/lib/degate
    mkdir $ROOT/etc
    mkdir $ROOT/usr/share/degate/icons
    mkdir $ROOT/usr/share/degate/glade
    mkdir $ROOT/usr/share/applications
    mkdir $ROOT/usr/share/pixmaps
    mkdir $ROOT/DEBIAN

    echo copy data into the tree
    cp plugins/*.so $ROOT/usr/lib/degate/
    cp glade/*.glade $ROOT/usr/share/degate/glade
    cp icons/*.png $ROOT/usr/share/degate/icons
    cp icons/degate_logo.png $ROOT/usr/share/pixmaps
    cp FreeSans.ttf $ROOT/usr/share/degate

    cp doc/man/degate.1 $ROOT/usr/share/man/man1/degate.1
    gzip -9 $ROOT/usr/share/man/man1/degate.1
    ln -s degate.1.gz $ROOT/usr/share/man/man1/degate_bin.1.gz

    cp changelog $ROOT/usr/share/doc/degate/
    gzip -9 $ROOT/usr/share/doc/degate/changelog

    cp degate $ROOT/usr/bin/degate_bin
    cat > $ROOT/usr/bin/degate <<EOF
#!/bin/sh
export DEGATE_HOME=/usr/share/degate
export DEGATE_PLUGINS=/usr/lib/degate
/usr/bin/degate_bin
EOF
    chmod 755 $ROOT/usr/bin/degate

    SIZE=`du -s $ROOT | awk '{print $1}'`

    cat > $ROOT/DEBIAN/control <<EOF
Package: $PACKAGE
Version: $VERSION
Section: electronics
Priority: optional
Architecture: $ARCH
Depends: zip, libgtkmm-2.4 | libgtkmm-2.4-1c2a, libglademm-2.4 | libglademm-2.4-1c2a, libglibmm-2.4 | libglibmm-2.4-1c2a, libmagick10 | libmagick9, libmagickcore1, libfreetype6, libc6
Installed-Size: $SIZE
Maintainer: Martin Schobert <martin@weltregierung.de>
Description: Degates' purpose is to aid  reverse engineering 
 of integrated circuits (ICs). Degate helps you to explore
 images from ICs. It matches logic gates  on the imagery
 given by graphical templates and it assists you in tracing
 circuit paths.
EOF

    cat > $ROOT/usr/share/doc/degate/copyright <<EOF
Degate - reverse engineering of logic gates

Package:

This Debian package was created by Martin Schobert.

Upstream:

Copyright 2008-2009 Martin Schobert <martin@weltregierung.de>


Licence:

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

On Debian systems, the text of the GPL is available in the file:
/usr/share/common-licenses/GPL-3 .

EOF

    cat > $ROOT/usr/share/applications/.desktop <<EOF
[Desktop Entry]
Encoding=UTF-8
Name=degate
Comment=Degate
Exec=degate
Icon=degate_logo.png
Terminal=false
Type=Application
Categories=Application;
EOF

    echo create a debian package
    fakeroot dpkg -b $ROOT
    lintian -i $ROOT.deb
}

enroll_deb_package $PACKAGE $VERSION $ARCH
enroll_src_package $PACKAGE $VERSION
