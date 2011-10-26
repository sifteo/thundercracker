
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "flashlayer.h"
#include <stdio.h>
#include <string.h>
#include <sifteo/machine.h>

//#define TEMP_TESTING

using namespace Sifteo;

AudioMixer::AudioMixer() :
    activeChannelMask(0),
    nextHandle(0)
{
}


void AudioMixer::init()
{
    for (int i = 0; i < NUM_CHANNELS; i++) {
        channels[i].decoder.init();
    }
}

#ifdef TEMP_TESTING // super hack for testing
#include "alert_test_data.h" // provides test_data[]
AudioMixer mixer;
void AudioMixer::test()
{
    mixer.init();
    AudioOutDevice::init(AudioOutDevice::kHz8000, &mixer);
    AudioOutDevice::start();

    handle_t handle;
    module_t id = 0;
    if (!mixer.play(id, &handle)) {
        while (1); // error
    }

    for (;;) {
        mixer.fetchData(); // this would normally be interleaved into the runtime
        if (!mixer.isPlaying(handle)) {
            if (!mixer.play(id, &handle)) {
                while (1); // error
            }
        }
//        ;
    }
}
#endif

/*
 * Slurp the data from all active channels into the out device's
 * buffer.
 * Called from the AudioOutDevice when it needs more data - not sure when this
 * will actually be happening yet.
 */
int AudioMixer::pullAudio(char *buffer, int numsamples)
{
    if (activeChannelMask == 0) {
        return 0;
    }

    memset(buffer, 0, numsamples);

    char *b, *bufend;
    AudioChannel *ch = &channels[0];
    uint32_t mask = activeChannelMask;
    int bytesMixed = 0;
    for (; mask != 0; mask >>= 1, ch++) {
        if ((mask & 1) == 0 || (ch->state & STATE_PAUSED)) {
            continue;
        }

        // if we have any decompressed bytes to mix, go for it
        if (ch->bytesToMix() >= 0) {
            b = buffer;
            bufend = buffer + numsamples;
            while (b != bufend) {
                *b++ += *ch->ptr++;
            }
            bytesMixed += numsamples;
        }

        // is this channel done?
        if (ch->endOfStream()) {
            if (!(ch->state & STATE_LOOP)) {
                stopChannel(ch);
            }
        }
    }

    return bytesMixed;
}

/*
 * Time to grab more audio data from flash.
 * Check whether each channel needs more data and grab if necessary.
 *
 * This currently assumes that it's being run on the main thread, such that it
 * can operate synchronously with data arriving from flash.
 */
void AudioMixer::fetchData()
{
    uint32_t mask = activeChannelMask;
    AudioChannel *ch = &channels[0];
    for (; mask != 0; mask >>= 1, ch++) {
        if ((mask & 1) && ch->bytesToMix() <= 0) {
            // do we need to decompress more data?
            ch->decompressedSize = ch->decoder.decodeFrame(ch->decompressBuf, sizeof(ch->decompressBuf));
            if (ch->decompressedSize > 0) {
                ch->ptr = ch->decompressBuf;
            }
        }
    }
}

void AudioMixer::setSoundBank(uintptr_t address)
{

}

bool AudioMixer::play(module_t mod, handle_t *handle, enum LoopMode loopMode)
{
    if (activeChannelMask == 0xFFFFFFFF) {
        return false; // no free channels
    }

    int channelIndex;
    uint32_t mask = (activeChannelMask);
    for (channelIndex = 0; mask & 1; channelIndex++) {
        mask >>= 1;
    }
    // already checked for no free channels above

    AudioChannel *ch = &channels[channelIndex];
    ch->state = (loopMode == LoopOnce) ? 0 : STATE_LOOP;
    // TODO: read info from soundbank, set channel's data pointer
#ifdef TEMP_TESTING // super hack for testing
    ch->decoder.setData((char*)test_data, sizeof(test_data));
#endif
    ch->ptr = ch->decompressBuf + sizeof(ch->decompressBuf); // mark it as empty
    ch->handle = nextHandle++;
    *handle = ch->handle;

    mask = activeChannelMask;
    Atomic::SetBit(activeChannelMask, channelIndex);
    if (mask == 0) {
//        outdev.start();
    }
    return true;
}

bool AudioMixer::isPlaying(handle_t handle)
{
    return channelForHandle(handle) != 0;
}

void AudioMixer::stop(handle_t handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        stopChannel(ch);
    }
}

void AudioMixer::stopChannel(AudioChannel *ch)
{
    int channelIndex = ch - channels;
    Atomic::ClearBit(activeChannelMask, channelIndex);
    if (activeChannelMask == 0) {
//        outdev.stop();
    }
}

void AudioMixer::pause(handle_t handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        ch->state |= STATE_PAUSED;
    }
}

void AudioMixer::resume(handle_t handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        ch->state &= ~STATE_PAUSED;
    }
}

uint32_t AudioMixer::pos(handle_t handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        ch = 0;
        // TODO - implement
    }
    return 0;
}

AudioMixer::AudioChannel* AudioMixer::channelForHandle(module_t handle)
{
    uint32_t mask = activeChannelMask;
    AudioChannel *ch = &channels[0];

    for (; mask != 0; ch++, mask >>= 1) {
        if ((mask & 0x1) && ch->handle == handle) {
            return ch;
        }
    }
    return 0;
}
