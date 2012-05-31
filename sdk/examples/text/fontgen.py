#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Very basic tool to generate a binary font from a TTF. Currently
# hardcodes a lot of things, so it's only really suitable for this demo.
#
# Assumes every glyph fits in an 8x8 box. Each glyph is encoded as
# an uncompressed 8-byte bitmap, preceeded by a one-byte escapement.
#

import ImageFont, ImageDraw, Image

FONT = "04B_03__.TTF"

font = ImageFont.truetype(FONT, 8)
glyphs = map(chr, range(ord(' '), ord('z')+1))

print """/*
 * 8x8 variable-width binary font, AUTOMATICALLY GENERATED
 * by fontgen.py from %s
 */

static const uint8_t font_data[] = {""" % FONT

for g in glyphs:
    width, height = font.getsize(g)
    assert width <= 8
    assert height <= 8

    im = Image.new("RGB", (8,8))
    draw = ImageDraw.Draw(im)
    draw.text((0, 0), g, font=font)

    bytes = [width]
    for y in range(8):
        byte = 0
        for x in range(8):
            if im.getpixel((x,y))[0]:
                byte = byte | (1 << x)
        bytes.append(byte)

    print "    %s" % "".join(["0x%02x," % x for x in bytes])

print "};"
