#!/usr/bin/env bash

in=$1
out=$2


size=$(stat -c '%s' "$1")

echo -n "xsfs:1:$size:" > $2
cat $1 >> $2

# Padding
pwgen 512 >> $2
