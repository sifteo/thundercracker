/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _FRONTEND_BMFONT_H
#define _FRONTEND_BMFONT_H

#include <stdint.h>

/*
 * Simple abstraction for BMFont files in memory
 */

class FrontendBMFont {
 public:
    struct Glyph {
        // From the BMFont file format
        uint32_t id;
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        int16_t xOffset;
        int16_t yOffset;
        int16_t xAdvance;
        uint8_t page;
        uint8_t channel;
    };

    static const Glyph *findGlyph(uint32_t id);
};

#endif
