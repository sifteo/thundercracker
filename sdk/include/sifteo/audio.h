/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef AUDIO_H_
#define AUDIO_H_

#include <stdint.h>
#include <sifteo/abi.h>
#include <sifteo/machine.h>

namespace Sifteo {

/*
 * A single module of audio - an encoded sample, a tracker style synth table, etc
 */
class AudioModule {
public:
    _SYSAudioModule sys;
};

class Audio {
public:
    static void enableChannel(_SYSAudioBuffer *buffer) {
        _SYS_audio_enableChannel(buffer);
    }

    static bool play(const Sifteo::AudioModule &mod, _SYSAudioHandle *handle, _SYSAudioLoopType loopMode = LoopOnce) {
        return _SYS_audio_play(&mod.sys, handle, loopMode);
    }

    static bool isPlaying(_SYSAudioHandle handle) {
        return _SYS_audio_isPlaying(handle);
    }

    static void stop(_SYSAudioHandle handle) {
        _SYS_audio_stop(handle);
    }

    static void pause(_SYSAudioHandle handle) {
        _SYS_audio_pause(handle);
    }
    static void resume(_SYSAudioHandle handle) {
        _SYS_audio_resume(handle);
    }

    static void setVolume(_SYSAudioHandle handle, int volume) {
        _SYS_audio_stop(handle);
    }
    static int volume(_SYSAudioHandle handle) {
        return _SYS_audio_volume(handle);
    }

    static uint32_t pos(_SYSAudioHandle handle) {
        return _SYS_audio_pos(handle);
    }
};

}; // namespace Sifteo

#endif // AUDIO_H_
