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
#include "speexdecoder.h"
#include "adpcmdecoder.h"
#include "flashlayer.h"


class AudioChannelSlot {
public:

    void init(_SYSAudioBuffer *b);

    bool isEnabled() const {
        return buf.isValid();
    }

    void play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode);
    int mixAudio(int16_t *buffer, unsigned len);

    _SYSAudioType channelType() const {
        return (_SYSAudioType) type;
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
    void fetchData();
    friend class AudioMixer;    // mixer can tell us to fetchData()

private:
    static const int STATE_PAUSED   = (1 << 0);
    static const int STATE_LOOP     = (1 << 1);
    static const int STATE_STOPPED  = (1 << 2);

    void onPlaybackComplete();
    static void fetchRaw(FlashStream &in, AudioBuffer &out);

    uint8_t state;
    uint8_t type;
    int16_t volume;
    _SYSAudioHandle handle;

    AudioBuffer buf;            // User-owned buffer for decompressed data
    FlashStream flStream;       // Location of compressed source data
    SpeexDecoder speexDec;      // Speex decoder state
    AdPcmDecoder adpcmDec;      // ADPCM decoder state
};

#endif /* AUDIOCHANNEL_H_ */
