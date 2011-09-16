#!/usr/bin/env python
#
# Byte histogram
#

import sys

hist = [0] * 256
total = 0

for fname in sys.argv[1:]:
    for byte in open(fname, 'rb').read():
        hist[ord(byte)] += 1
        total += 1

for i, count in enumerate(hist):
    t = count * 256.0 / total
    print "[%02x] %8.03f  %s" % (i, t, "." * int(t*10))

