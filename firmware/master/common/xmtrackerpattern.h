/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef XMTRACKERPATTERN_H_
#define XMTRACKERPATTERN_H_

#include <stddef.h>
#include "sifteo/abi.h"
#include "svmmemory.h"

struct XmTrackerNote {
    uint8_t note;
    uint8_t instrument;
    uint8_t volumeColumnByte;
    uint8_t effectType;
    uint8_t effectParam;
};

class XmTrackerPattern {
public:
    XmTrackerPattern() : song(0) { memset(&pattern, 0, sizeof(pattern)); }
    uint16_t nRows() { ASSERT(pattern.dataSize); return pattern.nRows; }
    void releaseRef() { ref.release(); }

    XmTrackerPattern *init(_SYSXMSong *pSong);
    void loadPattern(uint16_t i);
    void getNote(uint16_t row, uint8_t channel, struct XmTrackerNote &note);

    static const uint8_t kNoteOff = 97;
    static const uint8_t kNoNote = 0;
    static const uint8_t kNoInstrument = 0xFF;
    static const uint8_t kNoEffect = 0xFF;
    static const uint8_t kNoVolume = 0;
private:
    void nextNote(struct XmTrackerNote &note); // Pattern iterator

    _SYSXMSong *song;

    _SYSXMPattern pattern; // Current pattern
    FlashBlockRef ref;    // Dogpile-avoidance ref

    uint32_t noteOffset; // Index of next note within pattern
    uintptr_t offset;    // Offset of next note within pattern
};

#endif // XMTRACKERPATTERN_H_
