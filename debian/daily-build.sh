#!/bin/sh

# wrapper around dpkg-buildpackage to generate correct changelog
# use "debian/daily-build.sh -b" to create binary packages
# and "debian/daily-build.sh -S" to create a source package only

LC_ALL=C svn info 2> /dev/null | grep Revision | cut -d' ' -f2
version=$(LC_ALL=C svn info 2> /dev/null | grep Revision | cut -d' ' -f2)

# ensure correct directory
test -r debian/control || exit 1

rm debian/changelog
dch --create --empty --package mplayer -v 2:1.0~svn${version} "Daily build"

dpkg-buildpackage -us -uc "$@"
