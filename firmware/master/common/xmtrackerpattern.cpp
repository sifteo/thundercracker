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
    if (!pSong->nPatterns) {
        ASSERT(pSong->nPatterns);
        return 0;
    }

    song = pSong;

    memset(&pattern, 0, sizeof(pattern));

    return this;
}

void XmTrackerPattern::loadPattern(uint16_t i)
{
    if (!song) {
        LOG((LGPFX"Error: Can not load patterns without song "
             "(did you call XmTrackerPattern::init()?)\n"));
        ASSERT(song);
        memset(&pattern, 0, sizeof(pattern));
        return;
    }
    if (i >= song->nPatterns) {
        ASSERT(i < song->nPatterns);
        LOG((LGPFX"Error: Pattern %u is larger than song (%u patterns)\n",
             i, song->nPatterns));
        memset(&pattern, 0, sizeof(pattern));
        return;
    }

    SvmMemory::VirtAddr va = song->patterns + (i * sizeof(_SYSXMPattern));
    if (!SvmMemory::copyROData(pattern, va)) {
        // Fail in as many ways as possible!
        LOG((LGPFX"Error: Could not copy %p (length %lu)!\n",
             (void *)va, (long unsigned)sizeof(_SYSXMPattern)));
        ASSERT(false);
        memset(&pattern, 0, sizeof(pattern));
        return;
    }

    noteOffset = 0;
    offset = 0;
}

void XmTrackerPattern::getNote(uint16_t row, uint8_t channel, struct XmTrackerNote &note)
{
    if (!pattern.nRows) {
        LOG((LGPFX"Error: No pattern loaded, can't load notes!\n"));
        ASSERT(pattern.nRows);
        resetNote(note);
        return;
    }
    if (row >= pattern.nRows) {
        LOG((LGPFX"Error: Row %u is larger than pattern (%u rows)\n",
             row, pattern.nRows));
        ASSERT(row < pattern.nRows);
        resetNote(note);
        return;
    }
    if (channel >= song->nChannels) {
        LOG((LGPFX"Error: Channel %u is too large (song contains %u channels)\n",
             channel, song->nChannels));
        ASSERT(channel < song->nChannels);
        resetNote(note);
        return;
    }
    /* This is not an error condition, but can happen on an empty pattern.
     * Indicated by 64 rows, but no pData/dataSize.
     */
    if (!pattern.dataSize || !pattern.pData) {
        // TODO: test empty patterns
        LOG((LGPFX"Notice: Emitting empty note\n"));
        resetNote(note);
        return;
    }

    uint32_t noteIndex = row * song->nChannels + channel;

    if (noteIndex < noteOffset) {
        /* This happens on fxLoopPattern, but isn't terribly efficient.
         * Composers should avoid if reasonable (Workaround: more patterns).
         */
        if (row * song->nChannels + channel > 0)
            LOG((LGPFX"Notice: Had to seek(0) to get to note %u\n", row * song->nChannels + channel));
        offset = 0;
        noteOffset = 0;
    } else if (noteIndex > noteOffset) {
        /* This happens on fxPatternBreak, but isn't very efficient.
         * Composers should avoid if reasonable (Workaround: more patterns).
         */
        LOG((LGPFX"Notice: Reading past %u notes to get to target\n", row * song->nChannels + channel - noteOffset));
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
        LOG((LGPFX"Error: Could not copy %p (length %lu)!\n",
                 (void *)va, (long unsigned)sizeof(noteData)));
        ASSERT(false);
        resetNote(note);
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
        note.effectParam =      enc & (1 << 4) ? *(buf++) : kNoParam;
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

    // If the effect parameter is set but the effect was not, it was intended to be an arpeggio (effect 0)
    if (note.effectType == kNoEffect && note.effectParam != kNoParam) {
        note.effectType = 0;
    }

    // Clean up note
    if (note.note > kNoteOff)
        note.note = kNoNote;

    if (note.note == 0)
        note.note = kNoNote;

    // Clean up instrument
    if (note.instrument > 0 && note.instrument <= song->nInstruments)
        note.instrument--;
    else
        note.instrument = kNoInstrument;

    // Clean up volume
    if (note.volumeColumnByte <= 0x0F)
        note.volumeColumnByte = 0;

    if (note.volumeColumnByte >= 0x51 && note.volumeColumnByte <= 0x5F)
        note.volumeColumnByte = kNoVolume;

    if (note.volumeColumnByte >= 0xC0 && note.volumeColumnByte <= 0xEF)
        note.volumeColumnByte = 0;

    if (note.effectType != kNoEffect && note.effectParam == kNoParam)
        note.effectParam = 0;

    /* Users of this API should be able to check if a note is real
     * with < kNoteOff (97), so ensure it is never 0.
     */
    if (!note.note) {
        ASSERT(note.note);
        note.note = kNoNote;
    }
}
