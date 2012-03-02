/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOCHANNEL_H_
#define AUDIOCHANNEL_H_

#include <sifteo/audio.h>
#include <sifteo/machine.h>
#include <stdint.h>
#include "audiobuffer.h"
#include "flashlayer.h"

class SpeexDecoder;
class PCMDecoder;


class AudioChannelSlot {
public:
    static const int STATE_PAUSED   = (1 << 0);
    static const int STATE_LOOP     = (1 << 1);
    static const int STATE_STOPPED  = (1 << 2);

    AudioChannelSlot();
    void init(_SYSAudioBuffer *b);

    bool isEnabled() const {
        return buf.isValid();
    }

    void play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec);
    void play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, PCMDecoder *dec);
    int mixAudio(int16_t *buffer, unsigned len);

    _SYSAudioType channelType() const {
        return (_SYSAudioType) type;
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

    AudioBuffer buf;                // Decompressed data
    FlashStream flStream;           // Location of compressed source data
    FlashStreamBuffer<256> flBuf;   // Buffer for incoming compressed frames

    uint8_t state;
    uint8_t type;
    int16_t volume;
    _SYSAudioHandle handle;

    // TODO: Make an IAudioDecoder interface.
    union {
        SpeexDecoder *speex;
        PCMDecoder *pcm;
    } decoder;

    friend class AudioMixer;    // mixer can tell us to fetchData()
};

#endif /* AUDIOCHANNEL_H_ */
