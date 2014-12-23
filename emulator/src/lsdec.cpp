/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
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

/*
 * This is a simple Flash Loadstream decoder, based on a C prototype
 * of the decoder that was eventually written into the cube's 8051 firmware.
 * We can use this to instantly decompress and install assets during
 * simulation.
 */

#include <string.h>
#include "macros.h"
#include "lsdec.h"
#include "cube_flash_model.h"


LoadstreamDecoder::LoadstreamDecoder(uint8_t *buffer, uint32_t bufferSize)
    : buffer(buffer), bufferSize(bufferSize)
{
    ASSERT((bufferSize % Cube::FlashModel::SECTOR_SIZE) == 0);
    reset();
}

void LoadstreamDecoder::reset()
{
    memset(lut, 0, sizeof lut);
    state = S_OPCODE;
    flashAddr = 0;
}

void LoadstreamDecoder::setAddress(uint32_t addr)
{
    flashAddr = addr % bufferSize;
}

void LoadstreamDecoder::write8(uint8_t value)
{
    ASSERT(flashAddr < bufferSize);

    // Auto-erase
    if ((flashAddr % Cube::FlashModel::SECTOR_SIZE) == 0)
        memset(buffer + flashAddr, 0xFF, Cube::FlashModel::SECTOR_SIZE);

    // Endian swap
    buffer[flashAddr ^ 1] &= value;

    flashAddr++;
    if (flashAddr == bufferSize)
        flashAddr = 0;
}

void LoadstreamDecoder::write16(uint16_t value)
{
    write8(value);
    write8(value >> 8);
}

void LoadstreamDecoder::handleByte(uint8_t byte)
{
    switch (state) {

    case S_OPCODE: {
        opcode = byte;
        switch (opcode & OP_MASK) {

        case OP_LUT1:
            state = S_LUT1_COLOR1;
            return;

        case OP_LUT16:
            state = S_LUT16_VEC1;
            return;

        case OP_TILE_P0: {
            // Trivial solid-color tile, no repeats
            uint8_t i;
            uint16_t color = lut[byte & 0x0F];
            for (i = 0; i < 64; i++)
                write16(color);
            return;
        }

        case OP_TILE_P1_R4:
        case OP_TILE_P2_R4:
        case OP_TILE_P4_R4:
            counter = 64;
            rle1 = 0xff;
            rle2 = 0xfe;
            state = S_TILE_RLE4;
            return;

        case OP_TILE_P16:
            counter = 8;
            state = S_TILE_P16_MASK;
            return;

        case OP_SPECIAL:
            switch (opcode) {

            case OP_NOP:
                return;

            case OP_ADDRESS:
                state = S_ADDR_LOW;
                return;

            default:
                ASSERT(0);
                return;
            }
        }
    }

    case S_ADDR_LOW: {
        partial = byte;
        state = S_ADDR_HIGH;
        return;
    }

    case S_ADDR_HIGH: {
        uint32_t lat1_part = (byte >> 1) << 7;
        uint32_t lat2_part = (partial >> 1) << 14;
        uint32_t a21_part = (partial & 1) << 21;
        setAddress(lat1_part | lat2_part | a21_part);
        state = S_OPCODE;
        return;
    }

    case S_LUT1_COLOR1: {
        partial = byte;
        state = S_LUT1_COLOR2;
        return;
    }

    case S_LUT1_COLOR2: {
        lut[opcode & 0x0F] = partial | (byte << 8);
        state = S_OPCODE;
        return;
    }

    case S_LUT16_VEC1: {
        partial = byte;
        state = S_LUT16_VEC2;
        return;
    }

    case S_LUT16_VEC2: {
        lutVector = partial | (byte << 8);
        counter = 0;
        state = S_LUT16_COLOR1;
        return;
    }

    case S_LUT16_COLOR1: {
        partial = byte;
        state = S_LUT16_COLOR2;
        return;
    }

    case S_LUT16_COLOR2: {
        while (!(lutVector & 1)) {
            // Skipped LUT entry
            lutVector >>= 1;
            counter++;
        }
        lut[counter] = partial | (byte << 8);
        lutVector >>= 1;
        counter++;
        state = lutVector ? S_LUT16_COLOR1 : S_OPCODE;
        return;
    }   

    case S_TILE_RLE4: {
        uint8_t nybbleI = 2;

        // Loop over the two nybbles in our input byte, processing both identically.
        do {
            uint8_t nybble = byte & 0x0F;
            uint8_t runLength;

            if (rle1 == rle2) {
                // This is a run, and "nybble" is the run length.
                runLength = nybble;

                // Fill the whole byte, so we can do right-rotates below to our heart's content.
                nybble = rle1 | (rle1 << 4);

                // Disarm the run detector with a byte that can never occur in 'nybble'.
                rle1 = 0xF0;

            } else {
                // Not a run yet, just a literal nybble
                runLength = 1;
                rle2 = rle1;
                rle1 = nybble;
            }

            // Unpack each nybble at the current bit depth
            while (runLength) {
                runLength--;
                switch (opcode & OP_MASK) {

                case OP_TILE_P1_R4:
                    write16(lut[nybble & 1]);
                    nybble = (nybble >> 1) | (nybble << 7);
                    write16(lut[nybble & 1]);
                    nybble = (nybble >> 1) | (nybble << 7);
                    write16(lut[nybble & 1]);
                    nybble = (nybble >> 1) | (nybble << 7);
                    write16(lut[nybble & 1]);
                    nybble = (nybble >> 1) | (nybble << 7);
                    counter -= 4;
                    break;

                case OP_TILE_P2_R4:
                    write16(lut[nybble & 3]);
                    nybble = (nybble >> 2) | (nybble << 6);
                    write16(lut[nybble & 3]);
                    nybble = (nybble >> 2) | (nybble << 6);
                    counter -= 2;
                    break;

                case OP_TILE_P4_R4:
                    write16(lut[nybble & 0xF]);
                    counter--;
                    break;
            
                }
            }

            // Finished with this tile? Next.
            if ((int8_t)counter <= 0) {
                if (opcode & ARG_MASK) {
                    opcode--;
                    counter += 64;
                } else {
                    state = S_OPCODE;
                    return;
                }
            }

            byte = (byte >> 4) | (byte << 4);
        } while (--nybbleI);
        
        return;
    }

    case S_TILE_P16_MASK: {
        rle1 = byte;  // Mask byte for the next 8 pixels
        rle2 = 8;     // Remaining pixels in mask

        p16_emit_runs:
        do {
            if (rle1 & 1) {
                state = S_TILE_P16_LOW;
                return;
            }
            write16(lut[15]);
            rle1 = (rle1 >> 1) | (rle1 << 7);
        } while (--rle2);

        p16_next_mask:
        if (--counter) {
            state = S_TILE_P16_MASK;
        } else if (opcode & ARG_MASK) {
            opcode--;
            counter = 8;
            state = S_TILE_P16_MASK;
        } else {
            state = S_OPCODE;
        }

        return;
    }

    case S_TILE_P16_LOW: {
        write8(byte);
        partial = byte;
        state = S_TILE_P16_HIGH;
        return;
    }

    case S_TILE_P16_HIGH: {
        lut[15] = partial | (byte << 8);
        write8(byte);

        if (--rle2) {
            // Still more pixels in this mask.
            rle1 = (rle1 >> 1) | (rle1 << 7);
            goto p16_emit_runs;
        } else {
            // End of the mask byte or the whole tile
            goto p16_next_mask;
        }
    }
    }
}
