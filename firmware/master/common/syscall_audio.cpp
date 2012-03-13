/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for audio playback.
 */

#include <sifteo/abi.h>
#include "audiomixer.h"
#include "svmmemory.h"

extern "C" {

void _SYS_audio_enableChannel(struct _SYSAudioBuffer *buffer)
{
    if (SvmMemory::mapRAM(buffer, sizeof(*buffer)))
        AudioMixer::instance.enableChannel(buffer);
}

uint8_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop)
{
    _SYSAudioModule modCopy;
    if (SvmMemory::copyROData(modCopy, mod) && SvmMemory::mapRAM(h, sizeof(*h))) {
        return AudioMixer::instance.play(&modCopy, h, loop);
    }
    return false;
}

uint8_t _SYS_audio_isPlaying(_SYSAudioHandle h)
{
    return AudioMixer::instance.isPlaying(h);
}

void _SYS_audio_stop(_SYSAudioHandle h)
{
    AudioMixer::instance.stop(h);
}

void _SYS_audio_pause(_SYSAudioHandle h)
{
    AudioMixer::instance.pause(h);
}

void _SYS_audio_resume(_SYSAudioHandle h)
{
    AudioMixer::instance.resume(h);
}

int _SYS_audio_volume(_SYSAudioHandle h)
{
    return AudioMixer::instance.volume(h);
}

void _SYS_audio_setVolume(_SYSAudioHandle h, int volume)
{
    AudioMixer::instance.setVolume(h, volume);
}

uint32_t _SYS_audio_pos(_SYSAudioHandle h)
{
    return AudioMixer::instance.pos(h);
}

}  // extern "C"
