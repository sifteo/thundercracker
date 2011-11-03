
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "flashlayer.h"
#include <stdio.h>
#include <string.h>
#include <sifteo/machine.h>

using namespace Sifteo;

//statics
AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    activeChannelMask(0),
    nextHandle(0)
{
}

void AudioMixer::init()
{
    memset(channels, 0, sizeof(channels));
    for (int i = 0; i < _SYS_AUDIO_NUM_SAMPLE_CHANNELS; i++) {
        decoders[i].init();
    }
}

#if 0 // super hack for testing

#include "audio.gen.h" // provides blueshift

void AudioMixer::test()
{
    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz8000, &AudioMixer::instance);
    AudioOutDevice::start();

//    handle_t handle;
//    const Sifteo::AudioModule &mod = blueshift_8kHz;
    for (;;) {
//        if (!gMixer.isPlaying(handle)) {
//            if (!gMixer.play(mod, &handle)) {
//                while (1); // error
//            }
//        }
        AudioMixer::instance.fetchData(); // this would normally be interleaved into the runtime
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

    char *b;
    AudioChannel *ch = &channels[0];
    uint32_t mask = activeChannelMask;
    int bytesMixed = 0;
    for (; mask != 0; mask >>= 1, ch++) {
        if ((mask & 1) == 0 || (ch->isPaused())) {
            continue;
        }
        int mixed = ch->pullAudio((uint8_t*)buffer, numsamples);
        if (mixed > bytesMixed) {
            bytesMixed = mixed;
        }
        if (ch->endOfStream()) {
//            activeChannelMask
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
        if (mask & 1) {
            ch->fetchData();
        }
    }
}

//void AudioMixer::setSoundBank(uintptr_t address)
//{
//
//}

bool AudioMixer::play(const AudioModule &mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    if (activeChannelMask == 0xFFFFFFFF) {
        return false; // no free channels
    }

    int channelIndex = Intrinsic::CLZ(activeChannelMask);
    AudioChannel *ch = &channels[channelIndex];
    ch->state = (loopMode == LoopOnce) ? 0 : AudioChannel::STATE_LOOP;
//    ch->decoder.setData(mod.data, mod.size);
    ch->handle = nextHandle++;
    *handle = ch->handle;

    Atomic::SetBit(activeChannelMask, channelIndex);
    return true;
}

bool AudioMixer::isPlaying(_SYSAudioHandle handle)
{
    return channelForHandle(handle) != 0;
}

void AudioMixer::stop(_SYSAudioHandle handle)
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

void AudioMixer::pause(_SYSAudioHandle handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        ch->pause();
    }
}

void AudioMixer::resume(_SYSAudioHandle handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        ch->resume();
    }
}

void AudioMixer::setVolume(_SYSAudioHandle handle, int volume)
{
    (void)handle;
    (void)volume;
}

int AudioMixer::volume(_SYSAudioHandle handle)
{
    (void)handle; // TODO
    return 0;
}

uint32_t AudioMixer::pos(_SYSAudioHandle handle)
{
    if (AudioChannel *ch = channelForHandle(handle)) {
        ch = 0;
        // TODO - implement
    }
    return 0;
}

AudioChannel* AudioMixer::channelForHandle(_SYSAudioHandle handle)
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
