/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <sifteo/asset.h>
#include <sifteo/abi.h>

namespace Sifteo {


class AudioChannel {
public:

    AudioChannel() : handle(_SYS_AUDIO_INVALID_HANDLE)
    {}

    void init() {
        _SYS_audio_enableChannel(&buf);
    }

    bool play(const AssetAudio &mod, _SYSAudioLoopType loopMode = LoopOnce) {
        return _SYS_audio_play(&mod.sys, &handle, loopMode);
    }

    bool isPlaying() const {
        return _SYS_audio_isPlaying(handle);
    }

    void stop() {
        _SYS_audio_stop(handle);
    }

    void pause() {
        _SYS_audio_pause(handle);
    }

    void resume() {
        _SYS_audio_resume(handle);
    }

    void setVolume(int volume) {
        _SYS_audio_setVolume(handle, volume);
    }

    int volume() {
        return _SYS_audio_volume(handle);
    }

    uint32_t pos() {
        return _SYS_audio_pos(handle);
    }

private:
    // TODO - would be nice to have a _SYSAudioChannel type with these two
    // members, that serves as the object passed to the firmware.
    // Then, we wouldn't have to search for a channel. given a handle - we could
    // just calculate its offset within its array to look it up
    _SYSAudioHandle handle;
    _SYSAudioBuffer buf;
};

} // namespace Sifteo

#endif // AUDIO_H_
