/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef XMTRACKERPLAYER_H_
#define XMTRACKERPLAYER_H_

#include <stddef.h>
#include "sifteo/abi.h"
#include "xmtrackerpattern.h"
#include "svmmemory.h"
#include "macros.h"

struct XmTrackerChannel {
    struct _SYSXMInstrument instrument;
    struct XmTrackerNote note;
    uint16_t volume;
    uint32_t period;
    bool restart;

    inline uint8_t realNote() const {
        if (!instrument.sample.pData ||
            note.note == XmTrackerPattern::kNoteOff)
        {
            return 0;
        }

        return instrument.relativeNoteNumber + note.note;
    }
};

class XmTrackerPlayer {
public:
    static XmTrackerPlayer instance;

    static void mixerCallback() {
        if (instance.song.nPatterns == 0) return;

        instance.tick();
    }

    bool play(const struct _SYSXMSong *pSong);
    bool isPlaying() { return song.nPatterns > 0; }
    void stop();

    // TODO: future:
    // void muteChannel(uint8_t), unmuteChannel(uint8_t).

private:
    // Frequency and period computation
    static const uint8_t kLinearFrequencies = 0x01;
    static const uint8_t kAmigaFrequencies = 0x00;
    static const uint16_t AmigaPeriodTab[];
    static const uint32_t LinearFrequencyTab[];
    uint32_t getPeriod(int16_t note, int8_t finetune) const;
    uint32_t getFrequency(uint32_t period) const;

    // channels
    struct XmTrackerChannel channels[_SYS_AUDIO_MAX_CHANNELS];
    // voice ptrs
    _SYSXMSong song;

    uint8_t ticks;

    /* The naming of bpm and tempo follows the (unofficial) XM module file
     * spec. It can be both dissatisfying and confusing, but is retained here
     * for consistency.
     */
    uint8_t bpm;    // Speed of piece (yes, tempo)
    uint8_t tempo;  // Ticks per note

    // Playback positions
    uint16_t phrase;          // The current index into the pattern order table
    XmTrackerPattern pattern; // The current pattern
    uint16_t row;     // Current row within pattern, above

    void loadNextNotes();
    void commit();
    uint8_t patternOrderTable(uint16_t order);
    void tick();
};

#endif // XMTRACKERPLAYER_H_
