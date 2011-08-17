#!/usr/bin/env python

import Image, struct

def imageToRGB565(f, image, ckey=None):
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


f = open("flash.bin", "wb")

imageToRGB565(f, Image.open("assets/Owlbear1.png"), ckey=0x4FF5)    	# 4 frames at 0x000000
imageToRGB565(f, Image.open("assets/Owlbear2.png"), ckey=0x4FF5)    	# 4 frames at 0x020000
imageToRGB565(f, Image.open("assets/background.png"))  			# 128x512 at  0x040000
