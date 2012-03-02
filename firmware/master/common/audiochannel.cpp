/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/macros.h>
#include <sifteo/math.h>
#include <sifteo/audio.h>
#include "audiochannel.h"
#include "speexdecoder.h"
#include <limits.h>

using namespace Sifteo;

AudioChannelSlot::AudioChannelSlot() :
    state(0), volume(Audio::MAX_VOLUME)
{
    decoder.speex = NULL;
}

void AudioChannelSlot::init(_SYSAudioBuffer *b)
{
    buf.init(b);
    volume = Audio::MAX_VOLUME / 2;
}

void AudioChannelSlot::play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec)
{
    // if this is a sample & either the passed in decoder is null, or our
    // internal decoder is not null, we've got problems
    ASSERT(!(type == _SYS_Speex && dec == NULL));
    ASSERT(!(type == _SYS_Speex && decoder.speex != NULL));

    decoder.speex = dec;
    type = _SYS_Speex;
    state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    decoder.speex->setSource(reinterpret_cast<SvmMemory::VirtAddr>(mod->data), mod->dataSize);
}

void AudioChannelSlot::play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, PCMDecoder *dec)
{
    decoder.pcm = dec;
    type = _SYS_PCM;
    state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    decoder.pcm->setSource(reinterpret_cast<SvmMemory::VirtAddr>(mod->data), mod->dataSize);
}

int AudioChannelSlot::mixAudio(int16_t *buffer, unsigned len)
{
    ASSERT(!(state & STATE_STOPPED));

    int mixable = MIN(buf.readAvailable() / sizeof(*buffer), len);
    if (mixable > 0) {

        for (int i = 0; i < mixable; i++) {
            ASSERT(buf.readAvailable() >= 2);
            int16_t src = buf.dequeue() | (buf.dequeue() << 8);

            // Mix this sample, after volume adjustment, with the existing buffer contents
            int32_t sample = *buffer + ((src * (int32_t)this->volume) / Audio::MAX_VOLUME);
            
            // TODO - more subtle compression instead of hard limiter
            *buffer = Math::clamp(sample, (int32_t)SHRT_MIN, (int32_t)SHRT_MAX);
            buffer++;
        }

        switch (type) {

        case _SYS_Speex:
            assert(decoder.speex != 0);
            if (decoder.speex->endOfStream() && buf.readAvailable() == 0) {
                if (this->state & STATE_LOOP) {
                    this->decoder->setOffset(mod->offset, mod->size);
                }
            else {
                return -1;
            }
        }
        else if (pcmDecoder && pcmDecoder->endOfStream() && buf.readAvailable() == 0) {
            if (this->state & STATE_LOOP) {
                this->pcmDecoder->setOffset(mod->offset, mod->size);
            }
            else {
                return -1;
            }
        }
    }
    return mixable;
}

void AudioChannelSlot::fetchData()
{
    ASSERT(!(state & STATE_STOPPED));
    switch (mod->type) {
    case Sample: {
        if (decoder->endOfStream() || buf.writeAvailable() < SpeexDecoder::DECODED_FRAME_SIZE) {
            return;
        }
        
        unsigned bytesAvail;
        uint8_t *p = buf.reserve(SpeexDecoder::DECODED_FRAME_SIZE, &bytesAvail);
        uint32_t sz = decoder->decodeFrame(p, bytesAvail);
        ASSERT(sz == SpeexDecoder::DECODED_FRAME_SIZE || sz == 0);
        if (sz) {
            buf.commit(sz);
        }
    }
        break;
    case PCM: {
        if (pcmDecoder->endOfStream() || buf.writeAvailable() < PCMDecoder::FRAME_SIZE) {
            return;
        }
        
        unsigned bytesAvail;
        uint8_t *p = buf.reserve(PCMDecoder::FRAME_SIZE, &bytesAvail);
        uint32_t sz = pcmDecoder->decodeFrame(p, bytesAvail);
        ASSERT(sz <= PCMDecoder::FRAME_SIZE);
        buf.commit(sz);
    }
    default:
        break;
    }
}

void AudioChannelSlot::onPlaybackComplete()
{
    state |= STATE_STOPPED;
    this->decoder = 0;
    this->mod = 0;
}
