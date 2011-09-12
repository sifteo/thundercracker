#!/usr/bin/env python

import sys

print "static const uint8_t %s[] = {" % sys.argv[1]

for i, b in enumerate(open("%s.bin" % sys.argv[1], "rb").read()):
    print "0x%02x," % ord(b),
    if (i % 8) == 7:
        print

print "};"
