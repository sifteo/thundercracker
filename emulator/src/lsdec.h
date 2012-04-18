/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This is a simple Flash Loadstream decoder, based on a C prototype
 * of the decoder that was eventually written into the cube's 8051 firmware.
 * We can use this to instantly decompress and install assets during
 * simulation.
 */

#ifndef _LSDEC_H
#define _LSDEC_H

#include "macros.h"

namespace Cube {
    class FlashStorage;
}


class LoadstreamDecoder {
public:
    LoadstreamDecoder(uint8_t *buffer, uint32_t bufferSize);
    LoadstreamDecoder(Cube::FlashStorage &storage);

    void reset();
    void handleByte(uint8_t b);
    void handleBytes(uint8_t *byte, uint32_t count);
    void handleSVM(uint32_t va, uint32_t bytes);
    void setAddress(uint32_t addr);

private:
    void write8(uint8_t value);
    void write16(uint16_t value);
    
    uint8_t *buffer;
    uint32_t bufferSize;
    
    // Codec constants
    static const uint8_t LUT_SIZE       = 16;
    static const uint8_t OP_MASK        = 0xe0;
    static const uint8_t ARG_MASK       = 0x1f;
    static const uint8_t OP_LUT1        = 0x00;
    static const uint8_t OP_LUT16       = 0x20;
    static const uint8_t OP_TILE_P0     = 0x40;
    static const uint8_t OP_TILE_P1_R4  = 0x60;
    static const uint8_t OP_TILE_P2_R4  = 0x80;
    static const uint8_t OP_TILE_P4_R4  = 0xa0;
    static const uint8_t OP_TILE_P16    = 0xc0;
    static const uint8_t OP_SPECIAL     = 0xe0;
    static const uint8_t OP_ADDRESS     = 0xe1;

    // State machine states
    enum States {
        S_OPCODE = 0,
        S_ADDR_LOW,
        S_ADDR_HIGH,
        S_LUT1_COLOR1,
        S_LUT1_COLOR2,
        S_LUT16_VEC1,
        S_LUT16_VEC2,
        S_LUT16_COLOR1,
        S_LUT16_COLOR2,
        S_TILE_RLE4,
        S_TILE_P16_MASK,
        S_TILE_P16_LOW,
        S_TILE_P16_HIGH,
    };

    // Codec state
    uint32_t flashAddr;
    uint16_t lut[LUT_SIZE];
    uint16_t p16run;
    uint16_t lutVector;
    uint8_t opcode;
    uint8_t state;
    uint8_t partial;
    uint8_t counter;
    uint8_t rle1;
    uint8_t rle2;
};


#endif
