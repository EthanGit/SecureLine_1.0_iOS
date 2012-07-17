#!/bin/sh

AM_VERSION="1.10"
if ! type aclocal-$AM_VERSION 1>/dev/null 2>&1; then
	# automake-1.10 (recommended) is not available on Fedora 8
	AUTOMAKE=automake
	ACLOCAL=aclocal
else
	ACLOCAL=aclocal-${AM_VERSION}
	AUTOMAKE=automake-${AM_VERSION}
fi

libtoolize="libtoolize"
for lt in glibtoolize libtoolize15 libtoolize14 libtoolize13 ; do
        if test -x /usr/bin/$lt ; then
                libtoolize=$lt ; break
        fi
        if test -x /usr/local/bin/$lt ; then
                libtoolize=$lt ; break
        fi
        if test -x /opt/local/bin/$lt ; then
                libtoolize=$lt ; break
        fi
done
AUTOMAKE_FLAGS=""
case $libtoolize in
*glibtoolize)
	AUTOMAKE_FLAGS="-i"
	;;
esac

if test -d /opt/local/share/aclocal ; then
	ACLOCAL_ARGS="-I /opt/local/share/aclocal"
fi
if test -d /usr/local/share/aclocal ; then
	ACLOCAL_ARGS="-I /usr/local/share/aclocal"
fi

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
test "$srcdir" = "." && srcdir=`pwd`
ACLOCAL_ARGS="-I ${srcdir}/scripts ${ACLOCAL_ARGS}"

echo "Generating build scripts in amsip..."
set -x
$libtoolize --copy --force
$ACLOCAL  $ACLOCAL_ARGS

$AUTOMAKE --force-missing --add-missing --copy ${AUTOMAKE_FLAGS}
autoconf

