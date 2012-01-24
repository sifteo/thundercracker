
#include "audiomixer.h"
#include "audiooutdevice.h"
#include "flashlayer.h"
#include "cubecodec.h"  // TODO: This can be removed when the asset header structs are moved to a common file.
#include <stdio.h>
#include <string.h>
#include <sifteo.h>

using namespace Sifteo;

//statics
AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    enabledChannelMask(0),
    activeChannelMask(0),
    stoppedChannelMask(0),
    nextHandle(0),
    availableDecodersMask(ALL_DECODERS_AVAILABLE)
{
}

void AudioMixer::init()
{
    memset(channelSlots, 0, sizeof(channelSlots));
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
    // find the first disabled channel, init it and mark it as enabled
    for (unsigned idx = 0; idx < _SYS_AUDIO_MAX_CHANNELS; idx++) {
        if (!(enabledChannelMask & Intrinsic::LZ(idx))) {
            channelSlots[idx].init(buffer);
            Atomic::SetLZ(enabledChannelMask, idx);
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
 * Called from the AudioOutDevice when it needs more data, from within the
 * sample rate timer ISR.
 */
int AudioMixer::pullAudio(int16_t *buffer, int numsamples)
{
    // NOTE - *must* bail ASAP if we don't have anything to do. we really
    // want to minimize the work done in this ISR since it's happening so
    // frequently
    if (activeChannelMask == 0) {
        return 0;
    }
    memset(buffer, 0, numsamples * sizeof(*buffer));

    int samplesMixed = 0;
    // iterate through active channels that are not waiting to be stopped
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        AudioChannelSlot *ch = &channelSlots[idx];
        Atomic::ClearLZ(mask, idx);

        if (ch->isPaused()) {
            continue;
        }
        
        // Each channel individually mixes itself with the existing buffer contents
        int mixed = ch->mixAudio(buffer, numsamples);

        // Update size of overall mixed audio buffer
        if (mixed > samplesMixed) {
            samplesMixed = mixed;
        }
        else if (mixed < 0) {
            Atomic::SetLZ(stoppedChannelMask, idx);
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
    // first, handle any channels that have finished since we last rolled around
    while (stoppedChannelMask) {
        unsigned idx = Intrinsic::CLZ(stoppedChannelMask);
        stopChannel(&channelSlots[idx]);
        Atomic::ClearLZ(stoppedChannelMask, idx);
    }

    uint32_t mask = activeChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        channelSlots[idx].fetchData();
        Atomic::ClearLZ(mask, idx);
    }
}

/*
    Called from within Tasks::work to mix audio on the main thread, to be
    consumed by the audio out device.
*/
void AudioMixer::handleAudioOutEmpty(void *p) {
    AudioBuffer *buf = static_cast<AudioBuffer*>(p);

    unsigned bytesToWrite;
    int16_t *audiobuf = (int16_t*)buf->reserve(buf->writeAvailable(), &bytesToWrite);
    unsigned mixed = AudioMixer::instance.pullAudio(audiobuf, bytesToWrite / sizeof(int16_t));
    ASSERT(mixed < bytesToWrite * sizeof(int16_t));
    if (mixed > 0) {
        buf->commit(mixed * sizeof(int16_t));
    }
}

/*
    Read the details of this module out of flash.
*/
bool AudioMixer::populateModuleMetaData(struct _SYSAudioModule *mod)
{
    // TODO: can we tag these as already populated and avoid subsequent lookups?
    int size = 0;
    AssetIndexEntry *entry;

    entry = (AssetIndexEntry*)FlashLayer::getRegionFromOffset(0, FlashLayer::BLOCK_SIZE, &size);
    if (!entry) {
        LOG(("got null AssetIndexEntry from flash"));
        return false;
    }
    entry += mod->id;

    int offset = entry->offset;

    FlashLayer::releaseRegionFromOffset(0);

    SoundHeader *header;
    header = (SoundHeader*)FlashLayer::getRegionFromOffset(offset, sizeof(SoundHeader), &size);
    if (!header) {
        LOG(("Got null SoundHeader from flashlayer\n"));
        return false;
    }

    mod->offset = offset + sizeof(SoundHeader);
    mod->size = header->dataSize;
    mod->type = (_SYSAudioType)header->encoding;

    FlashLayer::releaseRegionFromOffset(offset);
    return true;
}

bool AudioMixer::play(struct _SYSAudioModule *mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode)
{
    if (enabledChannelMask == 0 || activeChannelMask == 0xFF000000) {
        return false; // no free channels
    }

    if (!populateModuleMetaData(mod)) {
        return false;
    }

    // find the next channel that's both enabled and inactive
    int idx;
    for (idx = 0; idx < _SYS_AUDIO_MAX_CHANNELS; idx++) {
        if ((enabledChannelMask & Intrinsic::LZ(idx)) &&
           !(activeChannelMask  & Intrinsic::LZ(idx))) {
            break;
        }
    }

    // does this module require a decoder? if so, get one
    SpeexDecoder *dec;
    PCMDecoder *pcmdec = 0;
    if (mod->type == Sample) {
        dec = getDecoder();
        if (dec == NULL) {
            LOG(("ERROR: No channels available.\n"));
            return false; // no decoders available
        }
    } else if (mod->type == PCM) {
        pcmdec = getPCMDecoder();
        if (pcmdec == NULL) {
            LOG(("ERROR: No channels available.\n"));
            return false; // no decoders available
        }
    }
    else {
        LOG(("ERROR: Unknown audio encoding.\n"));
        dec = 0;
    }

    AudioChannelSlot &ch = channelSlots[idx];
    ch.handle = nextHandle++;
    *handle = ch.handle;
    if (pcmdec) {
        ch.play(mod, loopMode, pcmdec);
    } else {
        ch.play(mod, loopMode, dec);
    }
    
    Atomic::SetLZ(activeChannelMask, idx);
    
    return true;
}

bool AudioMixer::isPlaying(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    return channelForHandle(handle, mask) != 0;
}

void AudioMixer::stop(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        stopChannel(ch);
    }
}

void AudioMixer::stopChannel(AudioChannelSlot *ch)
{
    unsigned channelIndex = ch - channelSlots;
    ASSERT(channelIndex < _SYS_AUDIO_MAX_CHANNELS);
    ASSERT(activeChannelMask & Intrinsic::LZ(channelIndex));

    // mark this channel's decoder as available again
    if (ch->channelType() == Sample) {
        unsigned decoderIndex = ch->decoder - decoders;
        ASSERT(decoderIndex < _SYS_AUDIO_MAX_CHANNELS);
        ASSERT(!(availableDecodersMask & Intrinsic::LZ(decoderIndex)));
        Atomic::SetLZ(availableDecodersMask, decoderIndex);
    }
    else if (ch->channelType() == PCM) {
        unsigned decoderIndex = ch->pcmDecoder - pcmDecoders;
        ASSERT(decoderIndex < _SYS_AUDIO_MAX_CHANNELS);
        ASSERT(!(availableDecodersMask & Intrinsic::LZ(decoderIndex)));
        Atomic::SetLZ(availableDecodersMask, decoderIndex);
    }
    ch->onPlaybackComplete();
    Atomic::ClearLZ(activeChannelMask, channelIndex);
}

void AudioMixer::pause(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        ch->pause();
    }
}

void AudioMixer::resume(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        ch->resume();
    }
}

void AudioMixer::setVolume(_SYSAudioHandle handle, int volume)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, enabledChannelMask)) {
        ch->volume = Math::clamp(volume, 0, (int)Audio::MAX_VOLUME);
    }
}

int AudioMixer::volume(_SYSAudioHandle handle)
{
    if (AudioChannelSlot *ch = channelForHandle(handle, enabledChannelMask)) {
        return ch->volume;
    }
    return 0;
}

uint32_t AudioMixer::pos(_SYSAudioHandle handle)
{
    uint32_t mask = activeChannelMask & ~stoppedChannelMask;
    if (AudioChannelSlot *ch = channelForHandle(handle, mask)) {
        ch = 0;
        // TODO - implement
    }
    return 0;
}

AudioChannelSlot* AudioMixer::channelForHandle(_SYSAudioHandle handle, uint32_t mask)
{
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        if (channelSlots[idx].handle == handle) {
            return &channelSlots[idx];
        }
        Atomic::ClearLZ(mask, idx);
    }
    return 0;
}

SpeexDecoder* AudioMixer::getDecoder()
{
    if (availableDecodersMask == 0) {
        return NULL;
    }

    unsigned idx = Intrinsic::CLZ(availableDecodersMask);
    Atomic::ClearLZ(availableDecodersMask, idx);
    return &decoders[idx];
}

PCMDecoder* AudioMixer::getPCMDecoder()
{
    if (availableDecodersMask == 0) {
        return NULL;
    }

    unsigned idx = Intrinsic::CLZ(availableDecodersMask);
    Atomic::ClearLZ(availableDecodersMask, idx);
    return &pcmDecoders[idx];
}
