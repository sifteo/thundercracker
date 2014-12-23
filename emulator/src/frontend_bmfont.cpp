/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
