#! /bin/sh
#
#

VERSION_STRING=CRMODELS_VERSION
VERSION_FILE=crmodels2.h
EXECS="bin/crmodels2 bin/cr2 bin/clasp-fe bin/hr"
PARSERDIR=../../mingw32-root

usage() {
	echo "Usage: build-mingw32.sh [--debug-version]"
}

DEBUG="no"
if [ "$1" = "--debug-version" ]; then
	DEBUG="yes"
	shift
fi

if [ $# != 0 ]; then
	usage
	exit 1
fi

VER=`grep 'define '$VERSION_STRING $VERSION_FILE | cut -d ' ' -f 3 | sed -e 's/"//g'`

if [ -f Makefile ]; then
	make distclean
fi
env CPPFLAGS="-I `pwd`/$PARSERDIR/include" LDFLAGS="-L `pwd`/$PARSERDIR/lib" mingw32-configure --bindir=`pwd`/$PARSERDIR/bin --libdir=`pwd`/$PARSERDIR/lib --includedir=`pwd`/$PARSERDIR/include
if [ "$DEBUG" = "yes" ]; then
	make all
else
	make release-bin
fi
for EXEC in $EXECS
do
	mv $EXEC $EXEC.exe
done

make distclean

echo ""
echo ""
echo ""
echo ""
for EXEC in $EXECS
do
	echo "$EXEC.exe is available in `pwd`"
done
echo ""
echo ""
