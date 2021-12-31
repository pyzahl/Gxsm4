#!/bin/sh 

# This script does all the magic calls to automake/autoconf and
# friends that are needed to configure a cvs checkout.  As described in
# the file HACKING you need a couple of extra tools to run this script
# successfully.
#
# If you are compiling from a released tarball you don't need these
# tools and you shouldn't use this script.  Just call ./configure
# directly.


PROJECT="GXSM4 - Gnome X Scanning Microscopy V4.0"

echo
echo "Checking for sourcedir"
	    
test -n "$srcdir" || srcdir=`dirname "$0"`
test -n "$srcdir" || srcdir=.

OLDDIR=`pwd`
cd $srcdir

(test -f configure.ac) || {
        echo "*** ERROR: Directory '$srcdir' does not look like the top-level project directory ***"
        exit 1
}

# shellcheck disable=SC2016
PKG_NAME=$(autoconf --trace 'AC_INIT:$1' configure.ac)

if [ "$#" = 0 -a "x$NOCONFIGURE" = "x" ]; then
        echo "*** WARNING: I am going to run 'configure' with no arguments." >&2
        echo "*** If you wish to pass any to it, please specify them on the" >&2
        echo "*** '$0' command line." >&2
        echo "" >&2
fi
echo
echo "running aclocal"
aclocal --install || exit 1

echo
echo "running intltoolize"
intltoolize --force --copy --automake || exit 1

echo 
echo "running autoreconf"
autoreconf --verbose --force --install || exit 1

echo
echo "running configure"
cd "$olddir"
if [ "$NOCONFIGURE" = "" ]; then
        $srcdir/configure "$@" || exit 1

        if [ "$1" = "--help" ]; then
                exit 0
        else
                echo "Configured $PKG_NAME" || exit 1
        fi
else
        echo "Skipping configure process."
fi

#
# CVS cannot save symlinks, we have to create them
# ourselves.
#

echo
echo "copying man-pages"
for i in gmetopng.1 rhkspm32topng.1 scalatopng.1
do
    if test -e "man/$i"; then
        echo "Fine, man-page $i exists!"
    else
        echo "creating $i"
        cp man/nctopng.1 man/$i
    fi
done

# end of creation of man-page symlinks.

echo
echo "Now type 'make' to compile the $PROJECT."
echo "Speed-up hint: try 'make -j' for fully utilizing a multi-core/-threadded/CPU system for example ;-)"
echo
