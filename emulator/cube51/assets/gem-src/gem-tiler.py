#!/usr/bin/env python
#
# The original gems are strips of 30x30 sprites. Pad to 32x32, and
# arrange each gem as a consecutive grouping of 4 tiles.
#

import Image

gems = [Image.open("01_gem%02d.png" % i).convert("RGB") for i in range(8)]
tilestrip = Image.new("RGB", (16, 8 * 5 * 4 * 16))
tilestrip.paste(gems[0].getpixel((0,0)))
y = 0

for gem in gems:
    for image in range(5):
        imy = image * 30

        tilestrip.paste(gem.crop((0, imy, 15, 15+imy)), (1, 1+y, 16, 16+y))
        y += 16
        tilestrip.paste(gem.crop((15, imy, 30, 15+imy)), (0, 1+y, 15, 16+y))
        y += 16
        tilestrip.paste(gem.crop((0,  15+imy, 15, 30+imy)), (1, 0+y, 16, 15+y))
        y += 16
        tilestrip.paste(gem.crop((15, 15+imy, 30, 30+imy)), (0, 0+y, 15, 15+y))
        y += 16

tilestrip.save("gem-tiles.png")
