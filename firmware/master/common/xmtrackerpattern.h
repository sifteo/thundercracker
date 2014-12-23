/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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
    uint16_t nRows() { return pattern.nRows; }
    void releaseRef() { ref.release(); }

    XmTrackerPattern *init(_SYSXMSong *pSong);
    bool loadPattern(uint16_t i);
    void getNote(uint16_t row, uint8_t channel, struct XmTrackerNote &note);

    static void resetNote(struct XmTrackerNote &note) {
        note.note = kNoteOff;
        note.instrument = kNoInstrument;
        note.volumeColumnByte = 0;
        note.effectType = kNoEffect;
        note.effectParam = 0;
    }

    static const uint8_t kMaxNote = 118;
    static const uint8_t kNoteOff = 97;
    static const uint8_t kNoNote = 0xFF;
    static const uint8_t kNoInstrument = 0xFF;
    static const uint8_t kNoEffect = 0xFF;
    static const uint8_t kNoParam = 0xFF;
    static const uint8_t kNoVolume = 0x55;
private:
    void nextNote(struct XmTrackerNote &note); // Pattern iterator

    _SYSXMSong *song;

    _SYSXMPattern pattern; // Current pattern
    FlashBlockRef ref;    // Dogpile-avoidance ref

    uint32_t noteOffset; // Index of next note within pattern
    uintptr_t offset;    // Offset of next note within pattern
};

#endif // XMTRACKERPATTERN_H_
