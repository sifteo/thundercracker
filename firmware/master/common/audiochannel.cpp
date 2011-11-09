
#include "audiochannel.h"
#include "speexdecoder.h"

void AudioChannel::init(_SYSAudioBuffer *b)
{
    buf.init(b);
}

void AudioChannel::play(const Sifteo::AudioModule &mod, _SYSAudioLoopType loopMode, SpeexDecoder *dec)
{
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
        if (this->endOfStream()) {
            this->decoder = 0;
            this->mod = 0;
            state |= STATE_STOPPED;
        }
    }
    return bytesToMix;
}

bool AudioChannel::endOfStream() const
{
    if (decoder) {
        return decoder->endOfStream() && buf.readAvailable() == 0;
    }
    return state & STATE_STOPPED;
}

void AudioChannel::fetchData()
{
    switch (mod->sys.type) {
    case Sample:
        while (buf.writeAvailable() >= SpeexDecoder::DECODED_FRAME_SIZE) {
            // TODO - better way to avoid copying data here?
            uint8_t buffer[SpeexDecoder::DECODED_FRAME_SIZE];
            uint32_t sz = decoder->decodeFrame(buffer, sizeof(buffer));
            if (!sz) {
                break; // TODO - we shouldn't be getting called here if decoder is empty
            }
            buf.write(buffer, sz);
        }
        break;
    default:
        break;
    }
}
