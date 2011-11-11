
#include <sifteo/macros.h>
#include "audiochannel.h"
#include "speexdecoder.h"

void AudioChannel::init(_SYSAudioBuffer *b)
{
    buf.init(b);
}

void AudioChannel::play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec)
{
    // if this is a sample & either the passed in decoder is null, or our
    // internal decoder is not null, we've got problems
    ASSERT(!(mod->type == Sample && dec == NULL));
    ASSERT(!(mod->type == Sample && this->decoder != NULL));

    this->decoder = dec;
    this->mod = mod;
    this->state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    if (this->decoder != 0) {
        this->decoder->setData(mod->data, mod->size);
    }
}

int AudioChannel::pullAudio(int16_t *buffer, int len)
{
    ASSERT(!(state & STATE_STOPPED));
    int bytesToMix = MIN(buf.readAvailable() / sizeof(*buffer), len);
    if (bytesToMix > 0) {
        for (int i = 0; i < bytesToMix; i++) {
            int16_t sample = buf.pop() | (buf.pop() << 8);
            *buffer += sample; //buf.pop(); // TODO - volume, limiting, compression, etc
            buffer++;
        }
        // if we have nothing buffered, and there's nothing else to read, we're done
        if (decoder->endOfStream() && buf.readAvailable() == 0) {
            if (this->state & STATE_LOOP) {
                this->decoder->setData(mod->data, mod->size);
            }
            else {
                return -1;
            }
        }
    }
    return bytesToMix;
}

void AudioChannel::fetchData()
{
    switch (mod->type) {
    case Sample: {
        if (decoder->endOfStream() || buf.writeAvailable() < SpeexDecoder::DECODED_FRAME_SIZE) {
            return;
        }
        // TODO - better way to avoid copying data here?
        uint8_t buffer[SpeexDecoder::DECODED_FRAME_SIZE];
        uint32_t sz = decoder->decodeFrame(buffer, sizeof(buffer));
        ASSERT(sz == SpeexDecoder::DECODED_FRAME_SIZE);
        buf.write(buffer, sz);
    }
        break;
    default:
        break;
    }
}

void AudioChannel::onPlaybackComplete()
{
    state |= STATE_STOPPED;
    this->decoder = 0;
    this->mod = 0;
}
