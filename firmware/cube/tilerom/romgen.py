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

        if len(self.tiles) > MAX_TILES:
            raise ValueError("Out of room in the tile map (%d tiles found, max is %d)"
                             % (len(self.tiles), MAX_TILES))

        # Unused tiles are reserved with all '1' bits, equivalent to blank flash memory.
        # This means we can still program those tiles later if we need to, on OTP parts.

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
            for plane in (tuple([x & 1 for x in pixels]),
                          tuple([x >> 1 for x in pixels])):
                self.tiles.append(plane)
                self.memo[plane] = True

    def renderTile(self, index):
        # Render a tile image, given its 14-bit index

        palBase = ((index >> 10) & 0xF) << 2
        mode = (index >> 9) & 1
        address = index & 0x1FF

        tile = Image.new("RGB", (TILE, TILE))
        for i in xrange(TILE*TILE):
            x = i % TILE
            y = i / TILE

            if mode:
                if (address & 0x7F) == 0x7F:
                    # DPL roll-over behavior is not defined.
                    # In our current implementation this will cause an advancement by one
                    # scanline, but apps shoudln't rely on that behaviour. So, this is a reserved
                    # index. Mark it as blank in the atlas.
                    index = 0
                else:
                    index = self.tiles[address][i] | (self.tiles[address + 1][i] << 1)
            else:
                index = self.tiles[address][i]

            tile.putpixel((x, y), self.palette[palBase + index])
    
        return tile

    def createAtlas(self, filename):
        # Generate an atlas image, which shows how all possible tile
        # indices will appear when rendered by our firmware. We save
        # an atlas dictionary, used by loadImage(), and we generate
        # a master image that can be used as a design reference or
        # as an input file for STIR.

        width = 128
        height = MAX_INDEX / width
        image = Image.new("RGB", (width*TILE, height*TILE))

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
        image = Image.open(filename).convert("RGB")
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
        arrays = [
            ('rom_tiles', self.swizzleTiles(self.tileBytes())),
            ('rom_palettes', self.paletteBytes()),
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

    def paletteBytes(self):
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

        for i, color in enumerate(self.palette):
            value = RGB565(*color)
            palIndex = i & 3

            bytes.append(0x78 + (palIndex*2))
            bytes.append(value & 0xFF)

            if palIndex:
                bytes.append(0x78 + (palIndex*2) + 1)
                bytes.append(value >> 8)

            if palIndex == 3:
                bytes.append(0x22)      # ret
                bytes.append(0x00)      # nop (pad to 16 bytes)

        return bytes

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
        "tilerom/img-battery.png",
        "tilerom/img-radio-0.png",
        "tilerom/img-radio-1.png",
        "tilerom/img-radio-2.png",
        "tilerom/img-radio-3.png",
        "tilerom/img-power.png",
        ):
        t.loadImage(img)

    t.saveCode("tilerom.c")
