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
    // channel ptrs
    // voice ptrs
    _SYSXMSong song;

    uint8_t ticks;
    uint8_t bpm;
    uint8_t tempo;

    // Playback positions
    uint16_t phrase;          // The current index into the pattern order table
    XmTrackerPattern pattern; // The current pattern
    uint16_t row;     // Current row within pattern, above

    void loadNextNotes();
    uint8_t patternOrderTable(uint16_t order);
    void tick();
};

#endif // XMTRACKERPLAYER_H_
