#!/usr/bin/env python
#
# Pure python hex dump
# M. Elizabeth Scott, 2009. This program is released into the public domain.
#

import sys
import cStringIO


def hexDump(src, dest=sys.stdout, bytesPerLine=16, wordSize=1, wordLE=False, addr=0, prefix=""):
    """A pretty standard hex dumper routine.
       Dumps the stream, string, or list 'src' to the stream 'dest'
       """

    if isinstance(src, list):
        src = ''.join(map(chr, src))

    if isinstance(src, str):
        src = cStringIO.StringIO(src)

    while 1:
        srcLine = src.read(bytesPerLine)
        if not srcLine:
            break

        # Address
        dest.write("%s%05X: " % (prefix,addr))
        addr += len(srcLine)

        # Hex values
        for i in xrange(bytesPerLine):
            # Little-endian word swap
            if wordLE:
                byte = i + wordSize - 1 - 2 * (i % wordSize)
            else:
                byte = i

            if byte < len(srcLine):
                dest.write("%02X" % ord(srcLine[byte]))
            else:
                dest.write("  ")

            if not (i+1) % wordSize:
                dest.write(" ")
        dest.write(" ")

        # ASCII representation
        for byte in srcLine:
            if ord(byte) >= 32 and ord(byte) < 127:
                dest.write(byte)
            else:
                dest.write(".")
        for i in xrange(bytesPerLine - len(srcLine)):
            dest.write(" ")
        dest.write("\n")


def main():
    if len(sys.argv) != 2:
        sys.stderr.write("usage: %s <file>\n" % sys.argv[0])
    else:
        hexDump(open(sys.argv[1]).read())

if __name__ == "__main__":
    main()
