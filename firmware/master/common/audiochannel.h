/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOCHANNEL_H_
#define AUDIOCHANNEL_H_

#include <sifteo/audio.h>
#include <stdint.h>
#include "audiobuffer.h"

class SpeexDecoder;

class AudioChannel {
public:
    static const int STATE_PAUSED   = (1 << 0);
    static const int STATE_LOOP     = (1 << 1);
    static const int STATE_STOPPED  = (1 << 2);

    AudioChannel() : mod(0), state(0), decoder(0)
    {}
    void init(_SYSAudioBuffer *b);

    bool isEnabled() const {
        return buf.isValid();
    }

    void play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec);
    int pullAudio(uint8_t *buffer, int len);

    _SYSAudioType channelType() const {
        ASSERT(mod != NULL);
        return mod->type;
    }

    void pause() {
        state |= STATE_PAUSED;
    }
    bool isPaused() const {
        return state & STATE_PAUSED;
    }
    void resume() {
        state &= ~STATE_PAUSED;
    }

protected:
    void fetchData();
    void onPlaybackComplete();

    AudioBuffer buf;
    const struct _SYSAudioModule *mod;
    uint8_t state;
    _SYSAudioHandle handle;
    SpeexDecoder *decoder;

    friend class AudioMixer;    // mixer can tell us to fetchData()
};

#endif /* AUDIOCHANNEL_H_ */
