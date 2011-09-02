#!/usr/bin/env python
#
# Simple 8x16 font generator
# M. Elizabeth Scott <beth@sifteo.com>
# 
# Copyright (c) 2011 Sifteo, Inc.
#

import Image
import ImageFont
import ImageDraw
import sys
from optparse import OptionParser


def fontgen(ttfFile, chars=map(chr, range(32,128)), tilew=8, tileh=16):
    im = Image.new("RGB", (tilew, tileh * len(chars)))

    ttfSize = tileh * 2
    while True:
        f = ImageFont.truetype(ttfFile, ttfSize)
        s = f.getsize("W")
        if s[0] <= tilew and s[1] <= tileh:
            break
        ttfSize -= 1
    print "Using font %r, size %r" % (ttfFile, ttfSize)

    for i, c in enumerate(chars):
        tile = Image.new("RGB", (tilew, tileh))
        tile.paste((255, 255, 255))

        draw = ImageDraw.Draw(tile)
        draw.text((0, 0), c, font=f, fill=(0,0,0))

        im.paste(tile, ((0, i * tileh, tilew, (i+1) * tileh)))

    return im


if __name__ == "__main__":
    parser = OptionParser(usage = "usage: %prog [options] font.ttf")
    parser.add_option("-t", "--tiles", dest="tiles",
                      help="write tiles as an image to FILE",
                      metavar="FILE.png", default="tiles.png")
    (options, args) = parser.parse_args()

    if len(args) != 1:
        parser.error("incorrect number of arguments")
    im = fontgen(args[0])
    im.save(options.tiles)
