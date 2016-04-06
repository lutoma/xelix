#!/usr/bin/env bash
out=$1
shift
a=($*)
infiles=$*
numfiles="${#a[@]}"

echo -n "xsfs:$numfiles:" > $out

for file in $infiles
do
	size=$(stat -c '%s' "$file")
	echo -n "$file-$size:" >> $out
done

echo -ne "\t" >> $out

cat $infiles >> $out

# Padding
pwgen 512 >> $out
