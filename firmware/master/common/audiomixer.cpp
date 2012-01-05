
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "flashlayer.h"
#include "cubecodec.h"  // TODO: This can be removed when the asset header structs are moved to a common file.
#include <stdio.h>
#include <string.h>
#include <sifteo/machine.h>
#include <sifteo/audio.h>
#include <sifteo/math.h>

using namespace Sifteo;

//statics
AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    enabledChannelMask(0),
    activeChannelMask(0),
    nextHandle(0),
    availableDecodersMask(ALL_DECODERS_AVAILABLE)
{
}

void AudioMixer::init()
{
    memset(channels, 0, sizeof(channels));
    for (int i = 0; i < _SYS_AUDIO_MAX_CHANNELS; i++) {
        decoders[i].init();
    }
}

/*
 * Userspace must supply a buffer for each channel they want to enable.
 * Once they do, the channel can be used in subsequent calls to play().
 */
void AudioMixer::enableChannel(struct _SYSAudioBuffer *buffer)
{
    if (enabledChannelMask >= ALL_CHANNELS_ENABLED) {
        return;
    }
    // find the first disabled channel, init it and mark it as enabled
    for (int i = 0; i < _SYS_AUDIO_MAX_CHANNELS; i++) {
        if (!(enabledChannelMask & (1 << i))) {
            Atomic::SetBit(enabledChannelMask, i);
            channels[i].init(buffer);
            return;
        }
    }
}

#if 0 // super hack for testing

#include "audio.gen.h" // provides blueshift
#include <speex/speex.h>

void AudioMixer::test()
{
    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

    Sifteo::AudioChannel channel;
    channel.init();

    const _SYSAudioModule &mod = spaceshipearth_16khz;
#if 0

#include <stdio.h>

    short decompressBuf[512];
    SpeexBits bits;
    void *decoderState;

    speex_bits_init(&bits);
    decoderState = speex_decoder_init(&speex_wb_mode);

    int framesize;
    speex_decoder_ctl(decoderState, SPEEX_GET_FRAME_SIZE, &framesize);

    int enhancer = 1;
    speex_decoder_ctl(decoderState, SPEEX_SET_ENH, &enhancer);

    FILE *fout = fopen("C:/Users/Liam/Desktop/speextesting/test_dec.raw", "wb");
    ASSERT(fout != 0);

    const uint8_t *data = mod.sys.data;
    int totalsize = mod.sys.size;
    int state = 0;
    int sz;

#if 1
    SpeexDecoder dec;
    dec.init();

    dec.setData(data, totalsize);

    while (!dec.endOfStream()) {
        sz = dec.decodeFrame((uint8_t*)decompressBuf, sizeof(decompressBuf));
        fwrite(decompressBuf, sizeof(uint8_t), sz, fout);
    }

#else

    while (state == 0) {
        sz = *data++;
        if (data - mod.sys.data >= totalsize) {
            break;
        }
        speex_bits_read_from(&bits, (char*)data, sz);
        state = speex_decode_int(decoderState, &bits, decompressBuf);
        data += sz;
        fwrite(decompressBuf, sizeof(short), framesize, fout);
    }
#endif

    fclose(fout);

    int t = state;

#endif
    if (!channel.play(mod)) {
        while (1); // error
    }
    for (;;) {
        if (!channel.isPlaying()) {
            if (!channel.play(mod)) {
                while (1); // error
            }
        }
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
int AudioMixer::pullAudio(int16_t *buffer, int numsamples)
{
    if (activeChannelMask == 0) {
        return 0;
    }

    memset(buffer, 0, numsamples * sizeof(*buffer));

    AudioChannelWrapper *ch = &channels[0];
    uint32_t mask = activeChannelMask;
    int samplesMixed = 0;
    for (; mask != 0; mask >>= 1, ch++) {
        if ((mask & 1) == 0 || (ch->isPaused())) {
            continue;
        }
        
        // Each channel individually mixes itself with the existing buffer contents
        int mixed = ch->mixAudio(buffer, numsamples);

        // Update size of overall mixed audio buffer
        if (mixed > samplesMixed) {
            samplesMixed = mixed;
        }
        else if (mixed < 0) {
            stopChannel(ch);
        }
    }
    return samplesMixed;
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
    // note - refer to activeChannelMask on each iteration,
    // as opposed to copying it to a local, in case it gets updated
    // during a call to fetchData()
    
    /*
     * XXX: This could be converted into a CLZ-style loop, as used in
     *      many other places in the firmware. That lets us iterate over
     *      only active channels, rather than interating over every
     *      preceeding inactive channel too. --beth
     */
    for (int i = 0; ; i++) {
        int m = activeChannelMask >> i;
        if (m == 0)
            return;
        if (m & 1)
            channels[i].fetchData();
    }
}

//void AudioMixer::setSoundBank(uintptr_t address)
//{
//
//}

#if 0
bool AudioMixer::play(const struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    if (enabledChannelMask == 0 || activeChannelMask == 0xFFFFFFFF) {
        return false; // no free channels
    }

    // find the next channel that's both enabled and inactive
    uint32_t activeMask = activeChannelMask;
    uint32_t enabledMask = enabledChannelMask;
    int idx;
    for (idx = 0; idx < _SYS_AUDIO_MAX_CHANNELS; idx++) {
        if ((enabledMask & (1 << idx)) &&
           ((activeMask  & (1 << idx))) == 0) {
            break;
        }
    }

    // does this module require a decoder? if so, get one
    SpeexDecoder *dec;
    if (mod->type == Sample) {
        dec = getDecoder();
        if (dec == NULL) {
            return false; // no decoders available
        }
    }
    else {
        dec = 0;
    }

    AudioChannelWrapper *ch = &channels[idx];
    ch->handle = nextHandle++;
    *handle = ch->handle;
    ch->play(mod, loopMode, dec);

    Atomic::SetBit(activeChannelMask, idx);
    return true;
}
#endif

bool AudioMixer::play(struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    int size = 0;
    AssetIndexEntry *entry;

    entry = (AssetIndexEntry*)FlashLayer::getRegionFromOffset(0, 512, &size);
    entry += mod->id;
    
    int offset = entry->offset;
    
    FlashLayer::releaseRegionFromOffset(0);
    
    SoundHeader *header;
    header = (SoundHeader*)FlashLayer::getRegionFromOffset(offset, sizeof(SoundHeader), &size);
    
    mod->offset = offset + sizeof(SoundHeader);
    mod->size = header->dataSize;
    
    FlashLayer::releaseRegionFromOffset(offset);    
    
    
    if (enabledChannelMask == 0 || activeChannelMask == 0xFFFFFFFF) {
        return false; // no free channels
    }

    // find the next channel that's both enabled and inactive
    uint32_t activeMask = activeChannelMask;
    uint32_t enabledMask = enabledChannelMask;
    int idx;
    for (idx = 0; idx < _SYS_AUDIO_MAX_CHANNELS; idx++) {
        if ((enabledMask & (1 << idx)) &&
           ((activeMask  & (1 << idx))) == 0) {
            break;
        }
    }

    // does this module require a decoder? if so, get one
    SpeexDecoder *dec;
    if (mod->type == Sample) {
        dec = getDecoder();
        if (dec == NULL) {
            return false; // no decoders available
        }
    }
    else {
        dec = 0;
    }

    AudioChannelWrapper *ch = &channels[idx];
    ch->handle = nextHandle++;
    *handle = ch->handle;
    ch->play(mod, loopMode, dec);

    Atomic::SetBit(activeChannelMask, idx);
    
    return true;
}

bool AudioMixer::isPlaying(_SYSAudioHandle handle)
{
    return channelForHandle(handle) != 0;
}

void AudioMixer::stop(_SYSAudioHandle handle)
{
    if (AudioChannelWrapper *ch = channelForHandle(handle)) {
        stopChannel(ch);
    }
}

void AudioMixer::stopChannel(AudioChannelWrapper *ch)
{
    int channelIndex = ch - channels;
    ASSERT(channelIndex < _SYS_AUDIO_MAX_CHANNELS);
    Atomic::ClearBit(activeChannelMask, channelIndex);
    if (ch->channelType() == Sample) {
        int decoderIndex = ch->decoder - decoders;
        ASSERT(decoderIndex < _SYS_AUDIO_MAX_CHANNELS);
        Atomic::SetBit(availableDecodersMask, decoderIndex);
    }
    ch->onPlaybackComplete();
}

void AudioMixer::pause(_SYSAudioHandle handle)
{
    if (AudioChannelWrapper *ch = channelForHandle(handle)) {
        ch->pause();
    }
}

void AudioMixer::resume(_SYSAudioHandle handle)
{
    if (AudioChannelWrapper *ch = channelForHandle(handle)) {
        ch->resume();
    }
}

void AudioMixer::setVolume(_SYSAudioHandle handle, int volume)
{
    if (AudioChannelWrapper *ch = channelForHandle(handle)) {
        ch->volume = Math::clamp(volume, 0, (int)Audio::MAX_VOLUME);
    }
}

int AudioMixer::volume(_SYSAudioHandle handle)
{
    if (AudioChannelWrapper *ch = channelForHandle(handle)) {
        return ch->volume;
    }
    return 0;
}

uint32_t AudioMixer::pos(_SYSAudioHandle handle)
{
    if (AudioChannelWrapper *ch = channelForHandle(handle)) {
        ch = 0;
        // TODO - implement
    }
    return 0;
}

AudioChannelWrapper* AudioMixer::channelForHandle(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask;
    AudioChannelWrapper *ch = &channels[0];

    for (; mask != 0; ch++, mask >>= 1) {
        if ((mask & 0x1) && ch->handle == handle) {
            return ch;
        }
    }
    return 0;
}

SpeexDecoder* AudioMixer::getDecoder()
{
    if (availableDecodersMask == 0) {
        return NULL;
    }

    /*
     * XXX: CLZ could do this in constant time, without the loop. --beth
     */
    for (int i = 0; i < _SYS_AUDIO_MAX_CHANNELS; i++) {
        if (availableDecodersMask & (1 << i)) {
            Atomic::ClearBit(availableDecodersMask, i);
            return &decoders[i];
        }
    }
    return NULL;
}
