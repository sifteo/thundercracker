/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "frontend_bmfont.h"


const FrontendBMFont::Glyph *FrontendBMFont::findGlyph(uint32_t id)
{
    /*
     * Very simplistic- linear search, one font, one texture page.
     * Nothing fancy going on. The BMFont format is pretty simple to
     * begin with, and here we're ignoring most of it.
     *
     * This is also assuming we're on a little-endian CPU that's fine
     * with unaligned accesses.
     */
     
    struct __attribute__ ((packed)) Header {
        uint8_t type;
        uint32_t size;
    };

    extern const uint8_t ui_font_data[];
    const uint8_t *p = ui_font_data + 4;
    const Header *hdr;
    
    // Find the 'chars' block (type 4)
    do {
        hdr = (const Header *)p;
        p += sizeof *hdr + hdr->size;
    } while (hdr->type != 4);
    
    // Find the character we're looking for (linear scan)
    const Glyph *chars = (const Glyph *) (hdr + 1);
    while (chars < (const Glyph *) p) {
        if (chars->id == id)
            return chars;
        chars++;
    }

    return 0;
}
