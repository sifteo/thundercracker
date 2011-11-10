
#include <sifteo/macros.h>
#include "audiochannel.h"
#include "speexdecoder.h"

void AudioChannel::init(_SYSAudioBuffer *b)
{
    buf.init(b);
}

void AudioChannel::play(const Sifteo::AudioModule &mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec)
{
    ASSERT(!(dec == NULL && mod.sys.type == Sample));
    this->decoder = dec;
    this->mod = &mod;
    this->state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    if (this->decoder != 0) {
        this->decoder->setData(mod.sys.data, mod.sys.size);
    }
}

int AudioChannel::pullAudio(uint8_t *buffer, int len)
{
    ASSERT(!(state & STATE_STOPPED));
    int bytesToMix = MIN(buf.readAvailable(), len);
    if (bytesToMix > 0) {
        for (int i = 0; i < bytesToMix; i++) {
            *buffer += buf.pop(); // TODO - volume, limiting, compression, etc
            buffer++;
        }
        // if we have nothing buffered, and there's nothing else to read, we're done
        if (decoder->endOfStream() && buf.readAvailable() == 0) {
            return -1;
        }
    }
    return bytesToMix;
}

void AudioChannel::fetchData()
{
    if (!mod || (state & STATE_STOPPED)) {
        return;
    }

    switch (mod->sys.type) {
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
