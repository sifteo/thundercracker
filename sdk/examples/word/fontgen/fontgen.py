#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Very basic tool to generate a binary font from a TTF. Currently
# hardcodes a lot of things, so it's only really suitable for this demo.
#
# Assumes every glyph fits in an 8x8 box. Each glyph is encoded as
# an uncompressed 8-byte bitmap, preceeded by a one-byte escapement.
#
# M. Elizabeth Scott <beth@sifteo.com>
# Copyright <c> 2011 Sifteo, Inc. All rights reserved.
#

import ImageFont, ImageDraw, Image

FONT = "ariblk.ttf"

font = ImageFont.truetype(FONT, 40)
glyphs = map(chr, range(ord('A'), ord('Z')+1))

index = 0
FRAME_WIDTH = 48
im = Image.new("RGBA", (FRAME_WIDTH * 26,64))
draw = ImageDraw.Draw(im)

for g in glyphs:
    width, height = font.getsize(g)
    #if width > 40 or height > 40:
        #print "character " + g + " is " + str(width) +", " +str(height)

    draw.text((4 + index * FRAME_WIDTH, 0), g, font=font, fill="Black")
    index += 1
# write to stdout
im.save("../font_2ltr.png", "PNG")    
