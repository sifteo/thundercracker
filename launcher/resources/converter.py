#!/usr/bin/env python
#
# Converter for special compile-time resources.
#
# This script is not invoked automatically as part of the build process,
# as it requires the Python Imaging Library.
#

import Image


def convertFB32(inFile, outFile, arrayName):
    im = Image.open(inFile)
    if im.mode != 'P':
        raise ValueError("Image must be palettized")

    out = open(outFile, 'w')
    out.write("// AUTOMATICALLY GENERATED by converter.py from %s\n\n" % inFile)
    out.write("static const uint16_t %s[] = {\n" % arrayName)

    word = 0
    for y in range(32):
        out.write("    ")
        for x in range(32):
            word = word | (im.getpixel((x,y)) << ((x & 3) * 4))
            if (x & 3) == 3:
                out.write("0x%04x," % word)
                word = 0
        out.write("\n")
    out.write("};\n")


def main():
    convertFB32("loading-bitmap.png", "loading-bitmap.h", "LoadingBitmap")


if __name__ == "__main__":
    main()
