/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "audiomixer.h"
#include "audiooutdevice.h"
#include "flash_blockcache.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "xmtrackerplayer.h"
#include "volume.h"

#ifdef SIFTEO_SIMULATOR
#   include "system.h"
#   include "system_mc.h"
#   include "mc_audiovisdata.h"
#else
#   include "sampleprofiler.h"
#endif


AudioMixer AudioMixer::instance;

AudioMixer::AudioMixer() :
    playingChannelMask(0),
    trackerCallbackInterval(0),
    trackerCallbackCountdown(0)
{}

void AudioMixer::init()
{
    uint32_t mask = playingChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        mask &= ~Intrinsic::LZ(idx);

        if (idx >= _SYS_AUDIO_MAX_CHANNELS) {
            ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);
            continue;
        }

        AudioChannelSlot &ch = channelSlots[idx];

        ch.stop();
    }

    XmTrackerPlayer::instance.init();
}

/*
 * Mix audio from flash into the audio device's buffer via each of the channels.
 *
 * This currently assumes that it's being run on the main thread, such that it
 * can operate synchronously with data arriving from flash.
 *
 * Returns true if any audio data was available. If so, we're guaranteed
 * to produce exactly 'numFrames' of data.
 */
bool AudioMixer::mixAudio(int *buffer, uint32_t numFrames)
{
    #ifdef SIFTEO_SIMULATOR
        MCAudioVisData::instance.mixerActive = active();
    #endif

    // Early out when we can quickly determine that no channels are playing.
    if (!active())
        return false;

    bool result = false;
    memset(buffer, 0, numFrames * sizeof(*buffer));

    uint32_t mask = playingChannelMask;
    while (mask) {
        unsigned idx = Intrinsic::CLZ(mask);
        mask ^= Intrinsic::LZ(idx);
        ASSERT(idx < _SYS_AUDIO_MAX_CHANNELS);

        AudioChannelSlot &ch = channelSlots[idx];

        if (ch.isStopped()) {
            Atomic::ClearLZ(playingChannelMask, idx);
            continue;
        }
        if (ch.isPaused()) {
            continue;
        }
        
        // Each channel individually mixes itself with the existing buffer contents
        result |= ch.mixAudio(buffer, numFrames);
    }
    
    return result;
}

/*
 * Called from within Tasks::work to mix audio on the main thread, to be
 * consumed by the audio out device.
 */
void AudioMixer::pullAudio(void *p)
{
    /*
     * Support audio in Siftulator, even in headless mode.
     *
     * In headless mode, we want to continue mixing even though
     * there's no buffer attached or no space in that buffer. We'll
     * either discard the mixed data, or if a waveout file is set we'll
     * end up logging the mixed audio data.
     */

    #ifdef SIFTEO_SIMULATOR
        const bool headless = SystemMC::getSystem()->opt_headless;
    #else
        const bool headless = false;
    #endif

    /*
     * Destination buffer, provided by the audio device.
     * If no audio device is available (yet) this will be NULL.
     */
    AudioBuffer *buf = static_cast<AudioBuffer*>(p);
    if (!buf && !headless)
        return;

    const int mixerVolume = Volume::systemVolume();
    ASSERT(mixerVolume <= _SYS_AUDIO_MAX_VOLUME);

    /*
     * In order to amortize the cost of iterating over channels, our
     * audio mixer operates on small arrays of samples at a time. We
     * give it a tiny buffer on the stack, which then flushes out
     * to the device's provided AudioBuffer.
     *
     * We're responsible for invoking the Tracker's callback
     * deterministically, exactly every 'trackerInterval' samples.
     * To do this, we may need to subdivide the blocks we request
     * from mixAudio(), or we may need to generate silent blocks.
     */

    #ifdef SIFTEO_SIMULATOR
        unsigned samplesLeft = headless ? SystemMC::suggestAudioSamplesToMix() : buf->writeAvailable();
    #else
        unsigned samplesLeft = buf->writeAvailable();
    #endif
    if (!samplesLeft)
        return;

    #ifndef SIFTEO_SIMULATOR
        SampleProfiler::SubSystem s = SampleProfiler::subsystem();
        SampleProfiler::setSubsystem(SampleProfiler::AudioPull);
    #endif

    const uint32_t trackerInterval = AudioMixer::instance.trackerCallbackInterval;
    uint32_t trackerCountdown = AudioMixer::instance.trackerCallbackCountdown;

    if (trackerInterval == 0) {
        // Tracker callbacks disabled. Avoid special-casing below by
        // assuming a countdown value that's effectively infinite.
        trackerCountdown = 0xFFFFFFFF;
    } else if (trackerCountdown == 0) {
        // Countdown should never be stored as zero when the timer is enabled.
        ASSERT(0);
        trackerCountdown = trackerInterval;
    }

    do {
        int blockBuffer[32];

        uint32_t blockSize = MIN(arraysize(blockBuffer), samplesLeft);
        blockSize = MIN(blockSize, trackerCountdown);
        ASSERT(blockSize > 0);

        bool mixed = AudioMixer::instance.mixAudio(blockBuffer, blockSize);

        if (!mixed) {
            /*
             * The mixer had nothing for us. Normally this means
             * we can early-out and let the device's buffer drain,
             * but if we're running the tracker, we need to keep
             * samples flowing in order to keep its clock advancing.
             * Generate silence.
             */
            if (trackerInterval == 0)
                break;
            else
                memset(blockBuffer, 0, sizeof blockBuffer);
        }

        trackerCountdown -= blockSize;
        samplesLeft -= blockSize;

        /*
         * Finish mixing our block of audio, by applying the system-wide
         * volume control and clamping to 16 bits.
         */

        int *ptr = blockBuffer;
        while (blockSize--) {
            int sample = (*(ptr++) * mixerVolume) / _SYS_AUDIO_MAX_VOLUME;
            int16_t sample16 = Intrinsic::SSAT(sample, 16);

            #ifdef SIFTEO_SIMULATOR
                // Log audio for --waveout
                SystemMC::logAudioSamples(&sample16, 1);
            #endif

            if (!headless)
                buf->enqueue(sample16);
        }

        if (!trackerCountdown) {
            ASSERT(trackerInterval);
            trackerCountdown = trackerInterval;
            XmTrackerPlayer::mixerCallback();
        }

    } while (samplesLeft);

    if (trackerInterval != 0) {
        // Write back local copy of Countdown, only if it's real.
        AudioMixer::instance.trackerCallbackCountdown = trackerCountdown;
    }

    #ifndef SIFTEO_SIMULATOR
        SampleProfiler::setSubsystem(s);
    #endif
}

bool AudioMixer::play(const struct _SYSAudioModule *mod,
    _SYSAudioChannelID ch, _SYSAudioLoopType loopMode)
{
    // NB: "mod" is a temporary contiguous copy of _SYSAudioModule in RAM.

    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return false;
    }

    // Already playing?
    if (isPlaying(ch))
        return false;

    AudioChannelSlot &slot = channelSlots[ch];
    slot.play(mod, loopMode);
    Atomic::SetLZ(playingChannelMask, ch);

    return true;
}

bool AudioMixer::isPlaying(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return false;
    }

    return (playingChannelMask & Intrinsic::LZ(ch)) != 0;
}

void AudioMixer::stop(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    channelSlots[ch].stop();
    Atomic::ClearLZ(playingChannelMask, ch);

    #ifdef SIFTEO_SIMULATOR
        MCAudioVisData::clearChannel(ch);
    #endif
}

void AudioMixer::pause(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    if (isPlaying(ch)) {
        channelSlots[ch].pause();
    }

    #ifdef SIFTEO_SIMULATOR
        MCAudioVisData::clearChannel(ch);
    #endif
}

void AudioMixer::resume(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    if (isPlaying(ch)) {
        channelSlots[ch].resume();
    }
}

void AudioMixer::setVolume(_SYSAudioChannelID ch, uint16_t volume)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    // XXX: should call setVolume
    channelSlots[ch].volume = clamp((int)volume, 0, (int)_SYS_AUDIO_MAX_VOLUME);
}

int AudioMixer::volume(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return -1;
    }

    return channelSlots[ch].volume;
}

void AudioMixer::setSpeed(_SYSAudioChannelID ch, uint32_t samplerate)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    channelSlots[ch].setSpeed(samplerate);
}

void AudioMixer::setPos(_SYSAudioChannelID ch, uint32_t ofs)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    channelSlots[ch].setPos(ofs);
}

uint32_t AudioMixer::pos(_SYSAudioChannelID ch)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return (uint32_t)-1;
    }

    // TODO - implement
    return 0;
}

void AudioMixer::setLoop(_SYSAudioChannelID ch, _SYSAudioLoopType loopMode)
{
    // Invalid channel?
    if (ch >= _SYS_AUDIO_MAX_CHANNELS) {
        ASSERT(ch < _SYS_AUDIO_MAX_CHANNELS);
        return;
    }

    channelSlots[ch].setLoop(loopMode);
}

void AudioMixer::setTrackerCallbackInterval(uint32_t usec)
{
    static const uint64_t kMicroSecondsPerSecond = 1000000;

    // Convert to frames
    trackerCallbackInterval = ((uint64_t)usec * (uint64_t)SAMPLE_HZ) / kMicroSecondsPerSecond;

    // Catch underflow. No one should ever need callbacks this often. Ever.
    ASSERT(usec == 0 || trackerCallbackInterval > 0);
    // But if we're not DEBUG, we may as well let it happen every sample. Gross.
    if (usec > 0 && trackerCallbackInterval == 0) {
        trackerCallbackInterval = 1;
    }

    trackerCallbackCountdown = trackerCallbackInterval;
}
