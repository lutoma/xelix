#!/usr/bin/env bash

# genconfigh.sh: Converts Kconfig file to config.h
# Copyright © 2016 Lukas Martini

# This file is part of Xelix.
#
# Xelix is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Xelix is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Xelix. If not, see <http://www.gnu.org/licenses/>.

utildir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
target="`dirname $utildir`/src/lib/config.h"

echo "#pragma once" > "$target"
echo "/* This file has been automatically generated. Please use 'make menuconfig' to change it. */" >> "$target"

while read p; do
	if [[ $p =~ ^\# ]] || [ -z "$p" ]; then
		continue
	fi

	pts=(${p//=/ })
	key=${pts[0]}
	value=${pts[1]}

	if [[ "$value" == "y" ]]; then
		echo "#define $key" >> "$target"
	else
		echo "#define $key $value" >> "$target"
	fi
done < "$utildir/kconfig/config"