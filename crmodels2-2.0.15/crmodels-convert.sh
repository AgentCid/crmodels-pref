#! /bin/sh
#
# Convert files for crmodels to input suitable for crmodels2+gringo/clasp.
#
# Usage: crmodels-convert.sh <file1> [<file2> [...]] > <output file>
#

if [ "$1" = "-h" -o $# -lt 1 ]; then
	echo "Usage: crmodels-convert.sh <file1> [<file2> [...]] > <output file>"
	exit 1
fi

# 1) hide -> #hide
# 2) neq(X,Y) -> X != Y

sed -e 's/^hide/#hide/' -e 's/neq(\([^\)]*\),\([^\)]*\))/\1!=\2/g' -e 's/gt(\([^\)]*\),\([^\)]*\))/\1>\2/g' -e 's/lt(\([^\)]*\),\([^\)]*\))/\1<\2/g' $*
