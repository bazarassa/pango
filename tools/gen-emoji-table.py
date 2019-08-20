#!/usr/bin/python

from __future__ import print_function, division, absolute_import
import sys
import os.path
from collections import OrderedDict

if len (sys.argv) != 2:
	print("usage: ./gen-emoji-table.py emoji-data.txt", file=sys.stderr)
	sys.exit (1)

f = open(sys.argv[1])
header = [f.readline () for _ in range(10)]

ranges = OrderedDict()
for line in f.readlines():
	line = line.strip()
	if not line or line[0] == '#':
		continue
	rang, typ = [s.strip() for s in line.split('#')[0].split(';')[:2]]

	rang = [int(s, 16) for s in rang.split('..')]
	if len(rang) > 1:
		start, end = rang
	else:
		start = end = rang[0]

	if typ not in ranges:
		ranges[typ] = []
	if ranges[typ] and ranges[typ][-1][1] == start - 1:
		ranges[typ][-1] = (ranges[typ][-1][0], end)
	else:
		ranges[typ].append((start, end))



print("/* == Start of generated table == */")
print("/*")
print(" * The following tables are generated by running:")
print(" *")
print(" *   ./gen-emoji-table.py emoji-data.txt")
print(" *")
print(" * on file with this header:")
print(" *")
for l in header:
	print(" * %s" % (l.strip()))
print(" */")
print()
print("#ifndef PANGO_EMOJI_TABLE_H")
print("#define PANGO_EMOJI_TABLE_H")
print()
print("#include <glib.h>")
print()
print("struct Interval {\n  gunichar start, end;\n};")

for typ,s in ranges.items():
	if typ not in ['Emoji',
		       'Emoji_Presentation',
		       'Emoji_Modifier',
		       'Emoji_Modifier_Base',
		       'Extended_Pictographic']: continue
	print()
	print("static const struct Interval _vogue_%s_table[] =" % typ)
	print("{")
	for pair in sorted(s):
		print("  {0x%04X, 0x%04X}," % pair)
	print("};")

print()
print("#endif /* PANGO_EMOJI_TABLE_H */")
print()
print("/* == End of generated table == */")
