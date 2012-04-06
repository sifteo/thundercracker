/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOCHANNEL_H_
#define AUDIOCHANNEL_H_

#include <sifteo/abi.h>
#include <stdint.h>
#include "machine.h"
#include "audiobuffer.h"
#include "audiosampledata.h"

class AudioChannelSlot {
public:
    AudioChannelSlot() { init(); }

    void init();

    void play(const struct _SYSAudioModule *module, _SYSAudioLoopType loopMode);

    _SYSAudioType channelType() const {
        return (_SYSAudioType)mod.type;
    }

    void pause() {
        state |= STATE_PAUSED;
    }

    bool isPaused() const {
        return (state & STATE_PAUSED) != 0;
    }

    bool isStopped() const {
        return (state & STATE_STOPPED) != 0;
    }

    void resume() {
        state &= ~STATE_PAUSED;
    }

    void stop() {
        state |= STATE_STOPPED;
    }

protected:
    uint32_t mixAudio(int16_t *buffer, uint32_t len);
    friend class AudioMixer;    // mixer can tell us to mixAudio()

private:
    static const int STATE_PAUSED   = (1 << 0);
    static const int STATE_LOOP     = (1 << 1);
    static const int STATE_STOPPED  = (1 << 2);

    void onPlaybackComplete();

    uint8_t state;
    int16_t volume;
    _SYSAudioHandle handle;

    struct _SYSAudioModule mod;
    AudioSampleData samples;
    uint32_t sampleNum;
};

#endif /* AUDIOCHANNEL_H_ */
