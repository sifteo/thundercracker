/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Syscalls for audio playback.
 */

#include <sifteo/abi.h>
#include "audiomixer.h"
#include "svmmemory.h"
#include "svmruntime.h"
#include "xmtrackerplayer.h"

extern "C" {

uint32_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioChannelID ch, enum _SYSAudioLoopType loop)
{
    _SYSAudioModule modCopy;
    
    if (!isAligned(mod)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return false;
    }
    if (!SvmMemory::copyROData(modCopy, mod)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }

    #ifdef SIFTEO_SIMULATOR
        if (XmTrackerPlayer::instance.isUsingChannel(ch)) {
            LOG(("MIXER: warning! audio channel %d is being used by the tracker\n", ch));
            // XXX: upgrade this to an assert
            // leaving as a warning for now to give people a chance to fix their business
//            ASSERT(false && "MIXER: audio channel is being used by the tracker");
        }
    #endif

    return AudioMixer::instance.play(&modCopy, ch, loop);
}

uint32_t _SYS_audio_isPlaying(_SYSAudioChannelID ch)
{
    return AudioMixer::instance.isPlaying(ch);
}

void _SYS_audio_stop(_SYSAudioChannelID ch)
{
    AudioMixer::instance.stop(ch);
}

void _SYS_audio_pause(_SYSAudioChannelID ch)
{
    AudioMixer::instance.pause(ch);
}

void _SYS_audio_resume(_SYSAudioChannelID ch)
{
    AudioMixer::instance.resume(ch);
}

int32_t _SYS_audio_volume(_SYSAudioChannelID ch)
{
    return AudioMixer::instance.volume(ch);
}

void _SYS_audio_setVolume(_SYSAudioChannelID ch, int32_t volume)
{
    AudioMixer::instance.setVolume(ch, volume);
}

void _SYS_audio_setSpeed(_SYSAudioChannelID ch, uint32_t hz)
{
    AudioMixer::instance.setSpeed(ch, hz);
}

uint32_t _SYS_audio_pos(_SYSAudioChannelID ch)
{
    return AudioMixer::instance.pos(ch);
}

uint32_t _SYS_tracker_play(const struct _SYSXMSong *song)
{
    if (!song)
        return XmTrackerPlayer::instance.play(0);

    if (!isAligned(song)) {
        SvmRuntime::fault(F_SYSCALL_ADDR_ALIGN);
        return false;
    }

    _SYSXMSong modCopy;
    if (!SvmMemory::copyROData(modCopy, song)) {
        SvmRuntime::fault(F_SYSCALL_ADDRESS);
        return false;
    }

    return XmTrackerPlayer::instance.play(&modCopy);
}

uint32_t _SYS_tracker_isStopped()
{
    return XmTrackerPlayer::instance.isStopped();
}

uint32_t _SYS_tracker_isPaused()
{
    return XmTrackerPlayer::instance.isPaused();
}

void _SYS_tracker_stop()
{
    XmTrackerPlayer::instance.stop();
}

void _SYS_tracker_setVolume(int volume, _SYSAudioChannelID ch)
{
    XmTrackerPlayer::instance.setVolume(volume, ch);
}

void _SYS_tracker_pause()
{
    XmTrackerPlayer::instance.pause();
}

void _SYS_tracker_setTempoModifier(int modifier)
{
    XmTrackerPlayer::instance.setTempoModifier(modifier);
}

void _SYS_tracker_setPosition(uint16_t phrase, uint16_t row)
{
    XmTrackerPlayer::instance.setPatternBreak(phrase, row);
}

}  // extern "C"
