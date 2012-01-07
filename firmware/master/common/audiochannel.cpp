
#include <sifteo/macros.h>
#include <sifteo/math.h>
#include <sifteo/audio.h>
#include "audiochannel.h"
#include "speexdecoder.h"
#include <limits.h>

using namespace Sifteo;

AudioChannelWrapper::AudioChannelWrapper() :
    mod(0), state(0), decoder(0), volume(Audio::MAX_VOLUME)
{
}

void AudioChannelWrapper::init(_SYSAudioBuffer *b)
{
    buf.init(b);
    volume = Audio::MAX_VOLUME / 2;
}

void AudioChannelWrapper::play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec)
{
    // if this is a sample & either the passed in decoder is null, or our
    // internal decoder is not null, we've got problems
    ASSERT(!(mod->type == Sample && dec == NULL));
    ASSERT(!(mod->type == Sample && this->decoder != NULL));

    this->decoder = dec;
    this->mod = mod;
    this->state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    if (this->decoder != 0) {
        this->decoder->setOffset(mod->offset, mod->size);
    }
}

void AudioChannelWrapper::play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, PCMDecoder *dec)
{
    this->pcmDecoder = dec;
    this->mod = mod;
    this->state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    if (this->pcmDecoder != 0) {
        this->pcmDecoder->setOffset(mod->offset, mod->size);
    }
}

int AudioChannelWrapper::mixAudio(int16_t *buffer, int len)
{
    ASSERT(!(state & STATE_STOPPED));

    int mixable = MIN(buf.readAvailable() / sizeof(*buffer), (unsigned)len);
    if (mixable > 0) {
        for (int i = 0; i < mixable; i++) {
            int16_t src = buf.dequeue() | (buf.dequeue() << 8);

            // Mix this sample, after volume adjustment, with the existing buffer contents
            int32_t sample = *buffer + ((src * (int32_t)this->volume) / Audio::MAX_VOLUME);
            
            // TODO - more subtle compression instead of hard limiter
            *buffer = Math::clamp(sample, (int32_t)SHRT_MIN, (int32_t)SHRT_MAX);
            buffer++;
        }
        // if we have nothing buffered, and there's nothing else to read, we're done
        if (decoder && decoder->endOfStream() && buf.readAvailable() == 0) {
            if (this->state & STATE_LOOP) {
                // FIXME: now using ID based assets
                //this->decoder->setData(mod->buf, mod->size);
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

void AudioChannelWrapper::fetchData()
{
    switch (mod->type) {
    case Sample: {
        if (decoder->endOfStream() || buf.writeAvailable() < SpeexDecoder::DECODED_FRAME_SIZE) {
            return;
        }
        
        /*
         * TODO - better way to avoid copying data here?
         * 
         * In the past I've written buffer APIs that had a two-phase 'write';
         * first you reserve() a particular number of bytes, recieving a pointer
         * to which you write the data. Then an atomic commit() operation updates
         * the buffer's write pointer. --beth
         */
        uint8_t buffer[SpeexDecoder::DECODED_FRAME_SIZE];
        uint32_t sz = decoder->decodeFrame(buffer, sizeof(buffer));
        ASSERT(sz == SpeexDecoder::DECODED_FRAME_SIZE);
        buf.write(buffer, sz);
    }
        break;
    case PCM: {
        if (pcmDecoder->endOfStream() || buf.writeAvailable() < PCMDecoder::FRAME_SIZE) {
            return;
        }
        
        uint8_t buffer[PCMDecoder::FRAME_SIZE];
        uint32_t sz = pcmDecoder->decodeFrame(buffer, sizeof(buffer));
        ASSERT(sz <= PCMDecoder::FRAME_SIZE);
        buf.write(buffer, sz);
    }
    default:
        break;
    }
}

void AudioChannelWrapper::onPlaybackComplete()
{
    state |= STATE_STOPPED;
    this->decoder = 0;
    this->mod = 0;
}
