#!/usr/bin/env python2
# coding: utf-8

# template.py: Autogenerate files based on the right template
# Copyright © 2011 Lukas Martini

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
# along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
#

import sys
import datetime
import pwd
import os

def error(message):
	print('Error: {0}'.format(message))
	exit(1)

def main():
	if len(sys.argv) != 2:
		error('Usage: {0} [file to create]'.format(sys.argv[0]))

	filename = sys.argv[1]
	extension = filename.split('.')

	if len(extension) < 2:
		error('This file does not seem to have an extension like .c or .py.')

	filenameNoExtension = extension[0]
	# Grab the last entry of the tuple
	extension = extension[-1]


	try:
		templateFile = open('templates/example.{0}'.format(extension), 'r')
	except IOError:
		error('There is no template for that language, try creating one.')

	try:
		target = open(filename, 'w')
	except IOError:
		error('Couldn\'t open the file you specified, make sure the directory exists and you have write rights for it')

	template = templateFile.read()
	templateFile.close()

	realname = pwd.getpwuid(os.getuid())[4].split(',')[0] # Rather dirty hack
	template = template.replace('<year>', str(datetime.datetime.now().year))
	template = template.replace('<filename>', filenameNoExtension)
	template = template.replace('<your name>', realname)

	if extension == 'c':
		template = template.replace('<own_header.h>', '"{0}.h"'.format(filenameNoExtension))
	
	target.write(template)
	target.close()


if __name__ == '__main__':
	main()
