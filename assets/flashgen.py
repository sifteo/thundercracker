#!/usr/bin/env python

import Image, struct

def imageToRGB565(f, image):
    for r, g, b in image.convert("RGB").getdata():
        f.write(struct.pack("<H",
                            ((r >> 3) << 11) |
                            ((g >> 2) << 5) |
                            (b >> 3)))

f = open("flash.bin", "wb")
imageToRGB565(f, Image.open("assets/Owlbear1.png"))
imageToRGB565(f, Image.open("assets/Owlbear2.png"))
