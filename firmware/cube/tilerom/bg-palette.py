#!/usr/bin/env python
#
# Generates an image with all legal 8-bit background colors
# Micah Elizabeth Scott <micah@misc.name>
# 
# Copyright (c) 2011 Sifteo, Inc.
#

import Image

def InverseRGB565(w):
    # Convert RGB565 back to RGB color
    return ( ((w >> 11) & 0x1F) * 255 / 31,
             ((w >> 5 ) & 0x3F) * 255 / 63,
             ((w >> 0 ) & 0x1F) * 255 / 31 )

im = Image.new("RGB", (16, 16))

for c in range(256):
    color = InverseRGB565(c | (c << 8))
    im.putpixel((c & 0xF, c >> 4), color)

im.save("bg-palette.png")
