#!/usr/bin/env python
#
# Generator script for ROM tileset.
# M. Elizabeth Scott <beth@sifteo.com>
# 
# Copyright (c) 2011 Sifteo, Inc.
#

import Image
import os.path
import struct

TILE = 8
MAX_TILES = 1 << 9
MAX_INDEX = 1 << 14


def RGB565(r, g, b):
    # Round to the nearest 5/6 bit color. Note that simple
    # bit truncation does NOT produce the best result!

    r5 = (r * 31 + 128) // 255;
    g6 = (g * 63 + 128) // 255;
    b5 = (b * 31 + 128) // 255;
    return (r5 << 11) | (g6 << 5) | b5

def InverseRGB565(w):
    # Convert RGB565 back to RGB color
    return ( ((w >> 11) & 0x1F) * 255 / 31,
             ((w >> 5 ) & 0x3F) * 255 / 63,
             ((w >> 0 ) & 0x1F) * 255 / 31 )
             
def squashColorTo8Bit(w):
    # Find the closest 8-bit color for the given RGB565 16-bit color.
    # Used for palette entry 0, where the MSB and LSB must be equal.

    best = None
    bestDistance = 1e10
    r1,g1,b1 = InverseRGB565(w)

    for candidate in range(256):
        candidate = candidate | (candidate << 8)
        r2,g2,b2 = InverseRGB565(candidate)
        diff = (r2-r1)**2 + (g2-g1)**2 + (b2-b1)**2
        if diff < bestDistance:
            best = candidate
            bestDistance = diff

    return best


class Tiler:
    def __init__(self):
        self.tiles = []
        self.memo = {}
        self.images = []
        self.atlas = {}

    def loadPalette(self, filename):
        self.palette = tuple(Image.open(filename).getdata())
        if len(self.palette) != 16 * 4:
            raise ValueError("Must have exactly 16 palettes, of 4 colors each")
        self.processPalette()

    def carveImage(self, image):
        width, height = image.size

        if (width % TILE) or (height % TILE):
            raise ValueError("Image of size %dx%d pixels is not a multiple of our %d pixel tile-size"
                             % (width, height, self.tileSize))
        width /= TILE
        height /= TILE

        for y in range(height):
            for x in range(width):
                yield (x*TILE, y*TILE, tuple(image.crop((x * TILE, y * TILE,
                                                         (x+1) * TILE, (y+1) * TILE)).getdata()))

    def loadTiles(self, filename):
        for x, y, tile in self.carveImage(Image.open(filename)):
            self.loadTile(tile)

        print "Collected %d tiles, out of %d maximum (%.02f%%)" % (
            len(self.tiles), MAX_TILES, len(self.tiles) * 100.0 / MAX_TILES)

        if len(self.tiles) > MAX_TILES:
            raise ValueError("Out of room in the tile map (%d tiles found, max is %d)"
                             % (len(self.tiles), MAX_TILES))

        # Unused tiles are reserved with all '1' bits, equivalent to blank flash memory.
        # This means we can still program those tiles later if we need to, on OTP parts.

        self.numDefinedTiles = len(self.tiles)
        while len(self.tiles) < MAX_TILES:
            self.tiles.append((1,) * (TILE * TILE))

    def loadTile(self, pixels):
        if 2 in pixels or 3 in pixels:
            self.loadTile4Color(pixels)
        else:
            self.loadTile2Color(pixels)
    
    def loadTile2Color(self, pixels):
        # This tile has one plane, and that plane can occur anywhere in memory.
        # Add it sequentially, and remember that we have this tile. If this tile
        # has occurred anywhere else (even as one plane from a 4-color tile) we can
        # omit it.

        if pixels not in self.memo:
            self.memo[pixels] = True
            self.tiles.append(pixels)

    def loadTile4Color(self, pixels):
        # We can skip this tile if the exact same 4-color tile has occurred already,
        # but otherwise we must always add both planes consecutively. Note that we
        # memoize both the 4-color version of this tile and each of its 2-color
        # planes.

        if pixels not in self.memo:
            self.memo[pixels] = True
            
            # 4-color tiles can't cross a 128-tile boundary
            if (len(self.tiles) & 0x7F) == 0x7F:
                print "Inserting dummy tile #%d" % len(self.tiles)
                self.tiles.append([0] * 64)

            for plane in (tuple([x & 1 for x in pixels]),
                          tuple([x >> 1 for x in pixels])):
                self.tiles.append(plane)
                self.memo[plane] = True

    def renderTile(self, index):
        # Render a tile image, given its 14-bit index

        palBase = ((index >> 10) & 0xF) << 2
        mode = (index >> 9) & 1
        address = index & 0x1FF

        tile = Image.new("RGBA", (TILE, TILE))

        # Default color to indicate areas of our atlas which are undefined
        tile.paste((255, 0, 255, 0))

        if (address & 0x7F) == 0x7F:
            # DPL roll-over behavior is not defined.
            # In our current implementation this will cause an advancement by one
            # scanline, but apps shoudln't rely on that behaviour. So, this is a reserved
            # index. Mark it as undefined in the atlas.
            pass

        elif mode:
            # 4-color
            if address + 1 < self.numDefinedTiles:
                for i in xrange(TILE*TILE):
                    x = i % TILE
                    y = i / TILE
                    index = self.tiles[address][i] | (self.tiles[address + 1][i] << 1)
                    tile.putpixel((x, y), self.reconstructedPalette[palBase + index])
    
        else:
            # 2-color
            if address < self.numDefinedTiles:
                for i in xrange(TILE*TILE):
                    x = i % TILE
                    y = i / TILE
                    index = self.tiles[address][i]
                    tile.putpixel((x, y), self.reconstructedPalette[palBase + index])    

        return tile

    def createAtlas(self, filename):
        # Generate an atlas image, which shows how all possible tile
        # indices will appear when rendered by our firmware. We save
        # an atlas dictionary, used by loadImage(), and we generate
        # a master image that can be used as a design reference or
        # as an input file for STIR.

        print "Creating atlas %r..." % filename

        width = 128
        height = MAX_INDEX / width
        image = Image.new("RGBA", (width*TILE, height*TILE))

        for y in range(height):
            for x in range(width):
                index = x + y*width
                tile = self.renderTile(index)
                raw = tuple(tile.getdata())

                if raw not in self.atlas:
                    # First tile in the atlas always gets priority
                    self.atlas[raw] = index

                image.paste(tile, (x*TILE, y*TILE))

        image.save(filename)

    def loadImage(self, filename):
        print "Processing image %r..." % filename

        image = Image.open(filename).convert("RGBA")
        indices = []

        for x, y, tile in self.carveImage(image):
            index = self.atlas.get(tile)
            if index is None:
                raise ValueError("Image %s has a tile at (%d, %d) which is not in the atlas!"
                                 % (filename, x, y))
            indices.append(index)

        name = os.path.splitext(os.path.split(filename)[1])[0].replace('-','_')
        self.images.append((name, image.size, indices))

    def saveCodeArrays(self, filename, arrays):
        f = open(filename, 'w')
        f.write("""
/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Tile ROM for Thundercracker cube firmware.
 * This file is AUTOMATICALLY GENERATED by romgen.py
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdint.h>

/*
 * ROM graphics data grows downward from the end of the code ROM.
 * Besides keeping our address layout simple, this makes it much
 * harder to exploit a firmware bug in order to read back our
 * (secret) firmware ROM.
 *
 * Tile data must be aligned on a 256-byte boundary. Currently
 * we place it in the last 4kB of ROM space.
 */
""".lstrip())

        addr = 0x4000
        for name, data in arrays:
            addr -= len(data)
            f.write("\nconst __code __at (0x%04x) uint8_t %s[] = {\n%s};\n"
                    % (addr, name, self.cByteArray(data)))

    def saveCode(self, filename):
        print "Generating code %r..." % filename

        arrays = [
            ('rom_tiles', self.swizzleTiles(self.tileBytes())),
            ('rom_palettes', self.paletteBytes),
            ]

        for name, size, indices in self.images:
            arrays.append((name, self.imageBytes(size, indices)))

        self.saveCodeArrays(filename, arrays)

    def cByteArray(self, bytes, width=16, indent="    "):
        # Format a list of byte values as a C array

        parts = []
        for i, b in enumerate(bytes):
            if (i % width) == 0:
                parts.append(indent)
            parts.append("0x%02x," % b)
            if (i % width) == width - 1 or i == len(bytes) - 1:
                parts.append("\n")
        return ''.join(parts)

    def processPalette(self):
        # Given a raw palette in self.palette[], create paletteBytes and
        # reconstructedPalette[].
        #
        # Palettes are stored in format very specific to the graphics
        # engine's implementation. During the inner loop of BG0_ROM, we
        # use one entire register bank as a fast copy of the currently
        # selected palette. It's valuable to be able to reload the palette
        # quickly and without changing dptr. A "mov Rn, #literal" instruction
        # only takes two bytes, so we can store the whole palette as a
        # field of small subroutines.
        #
        # As another multipurpose optimization, we assume that color 0 has
        # identical MSB and LSB. This lets us store only 7 bytes per palette,
        # letting us fit the whole palette into 16 bytes. This makes the
        # addressing code a good bit smaller. We also save time during
        # rendering by reloading BUS_PORT only once per pixel during these
        # (very common) background pixels.

        bytes = []
        reconstruct = []

        for i, color in enumerate(self.palette):
            value = RGB565(*color)
            palIndex = i & 3

            if palIndex == 0:
                value = squashColorTo8Bit(value)

            bytes.append(0x78 + (palIndex*2))
            bytes.append(value & 0xFF)

            if palIndex:
                bytes.append(0x78 + (palIndex*2) + 1)
                bytes.append(value >> 8)

            if palIndex == 3:
                bytes.append(0x22)      # ret
                bytes.append(0x00)      # nop (pad to 16 bytes)

            reconstruct.append(InverseRGB565(value))

        self.paletteBytes = bytes
        self.reconstructedPalette = reconstruct

    def tileBytes(self):
        bytes = []
        for tile in self.tiles:
            for y in range(TILE):
                byte = 0
                for x in range(TILE):
                    if tile[x + y*TILE]:
                        byte |= 1 << x
                bytes.append(byte)
        return bytes

    # Swizzling:
    #
    # Since we're free to choose the addressing scheme for these arrays
    # to be anything we want, and there's no reason to access them in order,
    # we choose an arrangement which is optimized to allow the graphics loops
    # to make as few calculations as possible. In particular, we really try
    # hard to avoid bit shifting, since there's no fast way to do it on the
    # 8051. This means taking advantage of bits in their existing alignment
    # within the byte we found them in.

    def swizzleTiles(self, bytes):
        # Address swizzling for tiles:
        #    0000 lttl tttt tttl -> 0000 tttt tttt tlll

        return [bytes[ ((i & 0x0600) << 1) |
                       ((i & 0x00FE) << 2) |
                       ((i & 0x0800) >> 9) |
                       ((i & 0x0100) >> 7) |
                       ((i & 0x0001) )] for i in range(len(bytes))]

    def getCommonMSB(self, indices):
        # If an image can use a single common most significant byte, figure out what
        # that byte is. If not, returns None.

        msb = None
        for i in indices:
            mi = i >> 8
            if msb is not None and mi != msb:
                return None
            msb = mi
        return msb

    def imageBytes(self, size, indices):
        # Image data, in general, is 16-bit. But if all the MSBs in the image are
        # identical, we can use a smaller 8-bit format. This is very common for images
        # that don't switch palettes or color modes.

        bytes = [size[0] // TILE, size[1] // TILE]
        msb = self.getCommonMSB(indices)

        if msb is None:
            # Needs to be 16-bit
            bytes.append(0xFF)
            for i in indices:
                bytes.append(i & 0xFF)
                bytes.append(i >> 8)

        else:
            # Can use smaller 8-bit format
            bytes.append(msb)
            for i in indices:
                bytes.append(i & 0xFF)

        return bytes


if __name__ == "__main__":
    t = Tiler()
    t.loadPalette("tilerom/src-palettes.png")
    t.loadTiles("tilerom/src-tiles.png")
    t.createAtlas("tilerom/tilerom-atlas.png")

    for img in (
        "tilerom/img-logo.png",
        "tilerom/img-trophy.png",
        "tilerom/img-battery.png",
        "tilerom/img-battery-bars-1.png",
        "tilerom/img-battery-bars-2.png",
        "tilerom/img-battery-bars-3.png",
        "tilerom/img-battery-bars-4.png",
        "tilerom/img-disconnected.png",
        "tilerom/img-disconnected-1.png",
        "tilerom/img-disconnected-2.png",
        "tilerom/img-disconnected-3.png",
        "tilerom/img-disconnected-4.png",
        ):
        t.loadImage(img)

    t.saveCode("src/tilerom.c")
