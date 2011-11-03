/*
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIOMIXER_H_
#define AUDIOMIXER_H_

#include <stdint.h>
#include "speexdecoder.h"
#include <sifteo/audio.h>

/*
 * Base class for AudioChannels
 */
class AudioChannel {
public:
    static const int STATE_PAUSED   = (1 << 0);
    static const int STATE_LOOP     = (1 << 1);
    static const int STATE_STOPPED  = (1 << 2);

    AudioChannel() : buf(0), state(0)
    {}
    void init(Sifteo::AudioBuffer *b) {
        buf = b;
    }
    int pullAudio(uint8_t *buffer, int len) {
        ASSERT(!(state & STATE_STOPPED));
        int bytesToMix = MIN(buf->bytesAvailable(), len);
        if (bytesToMix > 0) {
            for (int i = 0; i < bytesToMix; i++) {
                *buffer += buf->getU8(); // TODO - volume, limiting, compression, etc
                buffer++;
            }
            if (!buf->bytesAvailable()) {
                state |= STATE_STOPPED;
            }
        }
        return bytesToMix;
    }
    bool endOfStream() const {
        return state & STATE_STOPPED;
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
    void fetchData() {
        switch (audioType) {
        case Sample: {
                if (buf->bytesAvailable())
                    return;
                uint32_t sz = decoder->decodeFrame(buf->sys.data, sizeof(buf->sys.data));
                buf->setDataLen(sz);
            }
            break;
        default:
            break;
        }
    }
    Sifteo::AudioBuffer *buf;
    int state;
    enum _SYSAudioType audioType;
    _SYSAudioHandle handle;
    SpeexDecoder *decoder;
    friend class AudioMixer;    // mixer can tell us to fetchData()
};

class AudioMixer
{
public:
    static const int NUM_CHANNELS = 2;

    AudioMixer();

    static AudioMixer instance;

    void init();

    static void test();

    bool play(const Sifteo::AudioModule &mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode = LoopOnce);
    bool isPlaying(_SYSAudioHandle handle);
    void stop(_SYSAudioHandle handle);

    void pause(_SYSAudioHandle handle);
    void resume(_SYSAudioHandle handle);

    void setVolume(_SYSAudioHandle handle, int volume);
    int volume(_SYSAudioHandle handle);

    uint32_t pos(_SYSAudioHandle handle);

    bool active() const { return activeChannelMask; }

    int pullAudio(char *buffer, int numsamples);
    void fetchData();

private:
    uint32_t activeChannelMask;
    _SYSAudioHandle nextHandle;
    AudioChannel channels[_SYS_AUDIO_NUM_CHANNELS];
    SpeexDecoder decoders[_SYS_AUDIO_NUM_SAMPLE_CHANNELS];

    AudioChannel* channelForHandle(_SYSAudioHandle handle);
    void stopChannel(AudioChannel *ch);
};

#endif /* AUDIOMIXER_H_ */
