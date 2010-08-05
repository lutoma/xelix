#!/bin/sh

# usage:  tools/linecount.sh .

NUMBER=0
LINE=0
CHAR=0
count ()
{
	for temp in $1/* ; do
		if [ -d "$temp" ] ; then
			count "$temp"
		elif [ -f "$temp" ] ; then
			NUMBER=$(($NUMBER+1))
			LINE=$(($LINE+`wc -l $temp | awk '{print $1}'`))
			CHAR=$(($CHAR+`wc -m $temp | awk '{print $1}'`))
		fi
	done
}
count $1
echo "Found $NUMBER files containing $LINE lines and $CHAR characters"
