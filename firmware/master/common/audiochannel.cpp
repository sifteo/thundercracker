/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/macros.h>
#include <sifteo/math.h>
#include <sifteo/audio.h>
#include "svmmemory.h"
#include "audiochannel.h"
#include "speexdecoder.h"
#include <limits.h>

using namespace Sifteo;


void AudioChannelSlot::init(_SYSAudioBuffer *b)
{
    state = STATE_STOPPED;
    volume = Audio::MAX_VOLUME / 2;
    buf.init(b);
}

void AudioChannelSlot::play(const struct _SYSAudioModule *mod, _SYSAudioLoopType loopMode)
{
    SvmMemory::VirtAddr va = mod->pData;
    if (SvmMemory::initFlashStream(va, mod->dataSize, flStream)) {

        type = mod->type;
        state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;

        if (type == _SYS_Speex)
            speexDec.init();
    }
}

int AudioChannelSlot::mixAudio(int16_t *buffer, unsigned len)
{
    ASSERT(!(state & STATE_STOPPED));

    int mixable = MIN(buf.readAvailable() / sizeof *buffer, len);

    for (int i = 0; i < mixable; i++) {
        ASSERT(buf.readAvailable() >= 2);
        int16_t src = buf.dequeue() | (buf.dequeue() << 8);

        // Mix this sample, after volume adjustment, with the existing buffer contents
        int32_t sample = *buffer + ((src * (int32_t)volume) / Audio::MAX_VOLUME);
            
        // TODO - more subtle compression instead of hard limiter
        *buffer = Math::clamp(sample, (int32_t)SHRT_MIN, (int32_t)SHRT_MAX);
        buffer++;
    }

    return mixable;
}

void AudioChannelSlot::fetchData()
{
    ASSERT(!(state & STATE_STOPPED));

    if (flStream.eof())
        onPlaybackComplete();

    // onPlaybackComplete() may loop or not; test eof again.
    // This also handles edge cases like looping zero-length modules.
    if (flStream.eof())
        return;

    // Read and decompress audio data
    switch (type) {

    case _SYS_PCM:
        fetchRaw(flStream, buf);
        break;

    case _SYS_Speex:
        if (!speexDec.decodeFrame(flStream, buf)) {
            onPlaybackComplete();
            return;
        }
        break;

    default:
        break;
    }
}

void AudioChannelSlot::fetchRaw(FlashStream &in, AudioBuffer &out)
{
    uint32_t len = out.writeAvailable();
    uint8_t *buf = out.reserve(len, &len);
    len = in.read(buf, len);
    in.advance(len);
    out.commit(len);
}

void AudioChannelSlot::onPlaybackComplete()
{
    if (state & STATE_LOOP)
        flStream.seek(0);
    else
        stop();
}
