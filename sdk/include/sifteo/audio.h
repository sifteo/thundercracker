/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/asset.h>
#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @defgroup audio Audio
 *
 * @brief Interface to the audio mixer and the music tracker
 *
 * @{
 */

/**
 * @brief One mixer channel, capable of playing AudioAssets.
 *
 * Supported sample formats are ADPCM and standard PCM format. `stir` processes
 * your audio samples as part of the application packaging process.
 */
struct AudioChannel {
    _SYSAudioChannelID sys;
    typedef _SYSAudioChannelID AudioChannelID;

    /// A reserved ID, used to mark undefined AudioChannels
    static const AudioChannelID UNDEFINED = _SYS_AUDIO_INVALID_CHANNEL_ID;

    /// The maximum number of supported AudioChannels in the system.
    static const AudioChannelID NUM_CHANNELS = _SYS_AUDIO_MAX_CHANNELS;

    /// The maximum volume for an AudioChannel.
    static const int32_t MAX_VOLUME = _SYS_AUDIO_MAX_VOLUME;

    /// Loop modes available for use in play()
    enum LoopMode {
        UNDEF_LOOP = _SYS_LOOP_UNDEF,       /**< Default to the loop mode specified in Stir */
        ONCE = _SYS_LOOP_ONCE,              /**< Play once only (do not loop) */
        REPEAT = _SYS_LOOP_REPEAT,          /**< Repeat indefinitely */
    };

    /**
     * @brief Default constructor
     *
     * By default, an AudioChannel is initialized to a special UNDEFINED
     * value - initialize this value via init() before using the channel.
     */
    AudioChannel() : sys(UNDEFINED)
    {}

    /**
     * @brief Initialize an AudioChannel with a concrete value.
     *
     * If you use this constructor, there is no need to call init().
     * @param id must be in the range 0 to NUM_CHANNELS - 1
     */
    AudioChannel(AudioChannelID id) : sys(id)
    {}

    /**
     * @brief Initialize a channel by assignging it an ID.
     * @param id must be in the range 0 to NUM_CHANNELS - 1
     */
    void init(AudioChannelID id)
    {
        ASSERT(id < NUM_CHANNELS && "AudioChannel ID is invalid");
        sys = id;
    }

    /**
     * @brief Begin playback of a sample.
     * @param asset specifies the audio asset to playback.
     * @param loopMode specifies
     */
    bool play(const AssetAudio &asset, LoopMode loopMode = UNDEF_LOOP) const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        return _SYS_audio_play(&asset.sys, sys, (_SYSAudioLoopType) loopMode);
    }

    /**
     * @brief Is this channel currently playing a sample?
     */
    bool isPlaying() const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        return _SYS_audio_isPlaying(sys);
    }

    /**
     * @brief Stop playback of the current sample.
     *
     * Has no effect if a sample is not currently playing.
     */
    void stop() const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        _SYS_audio_stop(sys);
    }

    /**
     * @brief Pause the currently playing sample in this channel.
     *
     * Has no effect if a sample is not currently playing.
     */
    void pause() const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        _SYS_audio_pause(sys);
    }

    /**
     * @brief Resume playback on this channel.
     *
     * XXX: this may go away in favor of play()
     */
    void resume() const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        _SYS_audio_resume(sys);
    }

    /**
     * Sets the volume for this channel.
     * May be applied when the channel is either stopped or playing.
     * @param volume from 0 to MAX_VOLUME
     */
    void setVolume(int volume) const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        _SYS_audio_setVolume(sys, volume);
    }

    /**
     * @brief Sets the speed of this channel, in samples per second.
     *
     * To determine the default speed for an audio sample, you
     * can use AssetAudio::speed().
     */
    void setSpeed(unsigned hz) const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        _SYS_audio_setSpeed(sys, hz);
    }

    /**
     * @brief Get the current volume for this channel.
     */
    int volume() const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        return _SYS_audio_volume(sys);
    }

    uint32_t pos() const
    {
        ASSERT(sys < NUM_CHANNELS && "AudioChannel has invalid ID");
        return _SYS_audio_pos(sys);
    }
};

/**
 * @brief Playback interface for Tracker modules.
 *
 * Supported module format is standard XM. `stir` processes
 * your modules as part of the application packaging process, resulting
 * in an AssetTracker instance for each.
 */
struct AudioTracker {
    /**
     * @brief Begin playback of a module.
     * @param asset specifies the module for playback.
     */
    static bool play(const AssetTracker &asset) {
        return _SYS_tracker_play(&asset.song);
    }

    /**
     * @brief Resume playback of a paused module.
     */
    static bool resume() {
        return _SYS_tracker_play(0);
    }

    /**
     * @brief Pause playback of a module.
     */
    static void pause() {
        _SYS_tracker_pause();
    }

    /**
     * @brief Stop playback of the current module.
     *
     * Has no effect if a module is not currently playing.
     */
    static void stop() {
        _SYS_tracker_stop();
    }

    /**
     * @brief Is the tracker currently playing a module?
     */
    static bool isStopped() {
        return _SYS_tracker_isStopped();
    }

    /**
     * @brief Is the tracker currently playing a module?
     */
    static bool isPaused() {
        return _SYS_tracker_isPaused();
    }

    /**
     * @brief Scale volume for current channel.
     *
     * Must only be called while a module is playing.
     * @param volume from 0 to MAX_VOLUME.
     * @param ch specifies the index of the channel in the module. Leave empty to set global volume.
     */
    static void setVolume(int volume, _SYSAudioChannelID ch = -1)
    {
        ASSERT(volume >= 0 && volume <= _SYS_AUDIO_MAX_VOLUME);
        _SYS_tracker_setVolume(volume, ch);
    }
};

/**
 * @} endgroup Audio
 */

} // namespace Sifteo
