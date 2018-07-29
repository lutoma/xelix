#!/usr/bin/env python3
# This is all quite hacky, but it's decent enough for quick debugging

import sys
import re

allocs = dict()
kmalloc_re = re.compile('^kmalloc\: (?P<file>[^\:]+)\:(?P<line>\d+) (?P<func>\S+) (?P<orig_size>0x[\da-f]+).+ RESULT (?P<addr>0x[\da-f]+)$')
kfree_re = re.compile('^kfree\: (?P<file>[^\:]+)\:(?P<line>\d+) (?P<func>\S+) (?P<addr>0x[\da-f]+) size (?P<size>0x[\da-f]+)$')

def main():
	for line in sys.stdin.readlines():
		kfree_match = re.match(kfree_re, line.rstrip())
		kmalloc_match = re.match(kmalloc_re, line.rstrip())

		if not kfree_match and not kmalloc_match:
			if 'kfree' in line or 'kmallooc' in line:
				print('Could not match "{}"'.format(line))
			continue

		if kfree_match:
			try:
				del allocs[kfree_match.group('addr')]
			except KeyError:
				pass
			continue

		allocs[kmalloc_match.group('addr')] = {
			'file': kmalloc_match.group('file'),
			'line': kmalloc_match.group('line'),
			'func': kmalloc_match.group('func'),
			'size': kmalloc_match.group('orig_size'),
		}


	if not len(allocs):
		print('No memory leaks found')
	else:
		print('Potential memory leaks found')

	for addr, arr in allocs.items():
		print('{}:\tsize {}\t{} in {}:{}'.format(addr, arr['size'], arr['func'], arr['file'], arr['line']))

if __name__ == '__main__':
	main()
