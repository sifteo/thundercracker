/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "adpcmdecoder.h"


/**
 * We use the standard IMA ADPCM step sizes, even though our
 * codec isn't quite identical to IMA ADPCM.
 */
const uint16_t ADPCMDecoder::stepSizeTable[89] = {
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130,
    143, 157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449,
    494, 544, 598, 658, 724, 796, 876, 963, 1060, 1166, 1282, 1411,
    1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024, 3327, 3660, 4026,
    4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487,
    12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};


/**
 * Packed LUT for decoded nybble. Bits [7:0] are a signed coefficient for
 * the current step size, and bits [31:24] are the amount by which we update
 * the index. This split makes it easy to extract either half, as a signed
 * integer, using common ARM instructions. (Arithmetic shift, or 8-bit
 * sign extend).
 *
 * Multiply is cheap, table lookups are cheap!
 */
const int ADPCMDecoder::codeTable[16] = {
    int(0xFFFFFF01),
    int(0xFFFFFF03),
    int(0xFFFFFF05),
    int(0xFFFFFF07),
    int(0x00000209),
    int(0x0000040B),
    int(0x0000060D),
    int(0x0000080F),
    int(0xFFFFFFFF),
    int(0xFFFFFFFD),
    int(0xFFFFFFFB),
    int(0xFFFFFFF9),
    int(0x000002F7),
    int(0x000004F5),
    int(0x000006F3),
    int(0x000008F1),
};
