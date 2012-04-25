/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "xmtrackerpattern.h"
#include "macros.h"
#include "machine.h"

#define LGPFX "XmTrackerPattern: "

XmTrackerPattern *XmTrackerPattern::init(_SYSXMSong *pSong)
{
    ASSERT(pSong->nPatterns);
    song = pSong;

    memset(&pattern, 0, sizeof(pattern));

    return this;
}

void XmTrackerPattern::loadPattern(uint16_t i)
{
    ASSERT(song);
    ASSERT(i < song->nPatterns);

    SvmMemory::VirtAddr va = song->patterns + (i * sizeof(_SYSXMPattern));
    if (!SvmMemory::copyROData(pattern, va)) {
        // Fail in as many ways as possible!
        LOG((LGPFX"Could not copy %p (length %lu)!\n",
             (void *)va, (long unsigned)sizeof(_SYSXMPattern)));
        ASSERT(false);
        return;
    }

    noteOffset = 0;
    offset = 0;
}

void XmTrackerPattern::getNote(uint16_t row, uint8_t channel, struct XmTrackerNote &note)
{
    ASSERT(pattern.dataSize);
    ASSERT(row < pattern.nRows);
    ASSERT(channel < song->nChannels);

    if (pattern.nRows == 0) {
        // TODO
        LOG(("emitting empty note\n"));
        memset(&note, 0, sizeof(note));
        return;
    }

    uint32_t noteIndex = row * song->nChannels + channel;

    if (noteIndex < noteOffset) {
        // TODO: performance? how often does this happen?
        LOG((LGPFX"had to seek(0) to get to note %u\n", row * song->nChannels + channel));
        offset = 0;
        noteOffset = 0;
    } else if (noteIndex > noteOffset) {
        // TODO: this shouldn't happen often
        LOG((LGPFX"reading past %u notes to get to target\n", row * song->nChannels + channel - noteOffset));
    }

    while(noteIndex >= noteOffset) nextNote(note);
}

void XmTrackerPattern::nextNote(struct XmTrackerNote &note)
{
    /* The maximum amount of space a note can take up is 6 bytes, if the note
     * is encoded and still conatains all  the members of XmTrackerNote.
     * In practice a note should never take more than 5 bytes, since that is
     * the space an uncompressed note occupies, and stir verifies that patterns
     * are encoded as efficiently as possible.
     */
    uint8_t noteData[6];
    SvmMemory::VirtAddr va = pattern.pData + offset;
    if(!SvmMemory::copyROData(ref, noteData, va, sizeof(noteData))) {
        LOG((LGPFX"Could not copy %p (length %lu)!\n",
                 (void *)va, (long unsigned)sizeof(noteData)));
        ASSERT(false);

        // At worst, do no harm.
        memset(&note, 0, sizeof(note));
        return;
    }

    uint8_t *buf = noteData;
    if (*buf & 0x80) {
        uint8_t enc = *(buf++);
        // encoded note
        note.note =             enc & (1 << 0) ? *(buf++) : kNoNote;
        note.instrument =       enc & (1 << 1) ? *(buf++) : kNoInstrument;
        note.volumeColumnByte = enc & (1 << 2) ? *(buf++) : kNoVolume;
        note.effectType =       enc & (1 << 3) ? *(buf++) : kNoEffect;
        note.effectParam =      enc & (1 << 4) ? *(buf++) : 0;
        // If enc & 0x60 > 0 the pattern is likely corrupt, but follow Postel's Law.
        offset += Intrinsic::POPCOUNT(enc & 0x9F);
    } else {
        // unencoded note
        note.note =             *(buf++);
        note.instrument =       *(buf++);
        note.volumeColumnByte = *(buf++);
        note.effectType =       *(buf++);
        note.effectParam =      *(buf++);
        offset += 5;
    }
    noteOffset++;

    // Clean up values
    if (note.note > kNoteOff)
        note.note = kNoteOff;

    if (note.instrument >= song->nInstruments)
        note.instrument = kNoInstrument;

    if (note.volumeColumnByte <= 0x0F)
        note.volumeColumnByte = kNoVolume;

    if (note.volumeColumnByte >= 51 && note.volumeColumnByte <= 0x5F) {
        // LOG(("Undefined value for volume column byte (0x%02x), defaulting to %02x\n",
        //   note.volumeColumnByte, kNoVolume));
        note.volumeColumnByte = kNoVolume;
    }
    if (note.volumeColumnByte >= 0xC0 && note.volumeColumnByte <= 0xEF) {
        LOG(("Unsupported panning in volume column byte (0x%02x), defaulting to %02x\n",
             note.volumeColumnByte, kNoVolume));
        note.volumeColumnByte = kNoVolume;
    }

    // TODO: effects

    // A little late, but make sure we're not being rude.
    ASSERT(buf <= noteData + arraysize(noteData));
}
