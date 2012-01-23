import Image

def RGB565(r, g, b):
    #
    # Round to the nearest 5/6 bit color. Note that simple
    # bit truncation does NOT produce the best result!
    #
    r5 = (r * 31 + 128) // 255;
    g6 = (g * 63 + 128) // 255;
    b5 = (b * 31 + 128) // 255;
    return (r5 << 11) | (g6 << 5) | b5


def convert(filename):
    im = Image.open(filename).convert("RGB").quantize(16)

    for y in range(32):
        print "    ",
        for x in range(32):
            index = im.getpixel((x, y))
            if x & 1:
                byte = nybble | (index << 4)
                print "0x%02x," % byte,
            else:
                nybble = index
        print

    palette = im.getpalette()
    for i in range(16):
        if (i % 8) == 0:
            print "    ",
    
        offset = i * 3
        rgb = RGB565(*palette[offset:offset+3])

        print "0x%02x,0x%02x," % (rgb & 0xFF, rgb >> 8),
        if (i % 8) == 7:
            print

convert("loadimg.png")
