/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once

/*
 * This header needs to work in both userspace and non-userspace builds.
 */

#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @addtogroup assets
 * @{
 */

/**
 * @brief An audio asset, using any supported compression codec.
 *
 * `stir` generates an AssetAudio for each `sound` element in your assets.lua
 * configuration file, most often from a WAV source file.
 *
 * See the @ref audio and @ref asset_workflow guides for details.
 */

struct AssetAudio {
    _SYSAudioModule sys;

    /**
     * @brief Return the default speed for this audio asset, in samples per second.
     *
     * This can be used to calculate a new speed for AudioChannel::setSpeed().
     */
    unsigned speed() const {
        return sys.sampleRate;
    }

    /**
     * @brief Create an AssetAudio object programmatically, from uncompressed PCM data
     *
     * This creates an AssetAudio instance which references the specified
     * buffer of 16-bit uncompressed PCM samples. You can use this to wrap
     * dynamically synthesized audio data for playback on an AudioChannel.
     *
     * This is intended mostly for creating software "instruments" for
     * real-time audio synthesis, so we default to looping mode, and
     * there is no default sample rate or volume. These parameters
     * can all be set via AudioChannel at runtime.
     */
    static AssetAudio fromPCM(const int16_t *samples, unsigned numSamples)
    {
        const AssetAudio result = {{
            /* sampleRate  */  0,
            /* loopStart   */  0,
            /* loopEnd     */  numSamples,
            /* loopType    */  _SYS_LOOP_REPEAT,
            /* type        */  _SYS_PCM,
            /* volume      */  _SYS_AUDIO_DEFAULT_VOLUME,
            /* dataSize    */  numSamples * sizeof samples[0],
            /* pData       */  reinterpret_cast<uintptr_t>(samples),
        }};
        return result;
    }

    /// Templatized version of fromPCM(), for fixed-size sample arrays.
    template <typename T>
    static AssetAudio fromPCM(const T &samples) {
        return fromPCM(&samples[0], arraysize(samples));
    }
};

/**
 * @brief A Tracker module, converted from XM format by `stir`
 *
 * Tracker modules are great for supporting longer musical cues with
 * minimal storage requirements - it's efficient to have tracker modules
 * with several minutes of music, while several minutes of sample data
 * would be prohibitively large in many circumstances.
 *
 * To play an AssetTracker, see the AudioTracker.
 *
 * If you're looking for straight sample playback, often used for
 * sound effects, see AssetAudio.
 *
 * See the @ref audio and @ref asset_workflow guides for details.
 */

struct AssetTracker {
    _SYSXMSong song;

    /// The number of channels used by this tracker module.
    unsigned numChannels() const {
        return song.nChannels;
    }

    /// The number of patterns in this tracker module.
    unsigned numPatterns() const {
        return song.nPatterns;
    }

    /// The number of instruments in this tracker module.
    unsigned numInstruments() const {
        return song.nInstruments;
    }

    /// This tracker module's default tempo (ticks)
    unsigned tempo() const {
        return song.tempo;
    }

    /// This tracker module's default beats per minute (notes)
    unsigned bpm() const {
        return song.bpm;
    }
};

/**
 * @} end addtogroup assets
 */

};  // namespace Sifteo
