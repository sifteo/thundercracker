#!/usr/bin/env python

import Image, struct

def imageToRGB565(f, image, ckey=None, trunc=None):
    i = 0
    for r, g, b in image.convert("RGB").getdata():
        # Truncate to RGB-565. (If we wanted to be fancy, we could dither here)
        rgb16 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)

        # Our chromakey implementation can only compare the first byte
        # of each pixel, so only the LSB can be compared. If this
        # looks like our chromakey color but it actually isn't, fudge
        # the value just a bit so that we don't hit a false positive
        # during colorkey compositing.

        if ckey and rgb16 != ckey and (rgb16 & 0xFF) == (ckey & 0xFF):
            rgb16 -= 1
        
        # Always little-endian
        f.write(struct.pack("<H", rgb16))
        i += 1
        if trunc and i >= trunc:
            return

f = open("flash.bin", "wb")

# 0x000000 - 0x03FFFF : Owlbear sprites, 8 frames at 128x128
f.seek(0)
imageToRGB565(f, Image.open("assets/Owlbear1.png"), ckey=0x4FF5)
imageToRGB565(f, Image.open("assets/Owlbear2.png"), ckey=0x4FF5)

# 0x040000 - 0x067FFF : Background image, 128x512 with an extra 128 lines of wrap room (XXX)
f.seek(0x40000)
imageToRGB565(f, Image.open("assets/background.png"))
imageToRGB565(f, Image.open("assets/background.png"), trunc=128*128)

# 0x068000 - 0x087FFF : Tile data, 16x16 mode, 256 tiles (Must be 16k-aligned)
f.seek(0x68000)
imageToRGB565(f, Image.open("assets/gem-tiles.png"))

# 0x088000 - 0x089FFF : Four 32x32 monster sprites
f.seek(0x88000)
imageToRGB565(f, Image.open("assets/monsters.png"))

# 0x08c000 - 0x08DFFF : 64x64 terrain background
f.seek(0x8c000)
imageToRGB565(f, Image.open("assets/terrain64.png"))
