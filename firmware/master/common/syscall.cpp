/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementations for all syscall handlers.
 *
 * This is our front line of defense against buggy or malicious game
 * code, so here is where we need to carefully validate all input
 * values. The object implementations past this level don't
 * necessarily validate their input.
 */

#include <sifteo/abi.h>
#include "radio.h"
#include "cube.h"
#include "runtime.h"
#include "vram.h"
#include "neighbors.h"
#include "accel.h"
#include "audiomixer.h"

extern "C" {

struct _SYSEventVectors _SYS_vectors;

void _SYS_exit(void)
{
    Runtime::exit();
}

void _SYS_yield(void)
{
    Radio::halt();
    Event::dispatch();
}

void _SYS_paint(void)
{
    CubeSlot::paintCubes(CubeSlot::vecEnabled);
    Event::dispatch();
}

void _SYS_finish(void)
{
    CubeSlot::finishCubes(CubeSlot::vecEnabled);
    Event::dispatch();
}

void _SYS_ticks_ns(int64_t *nanosec)
{
    if (Runtime::checkUserPointer(nanosec, sizeof *nanosec))
        *nanosec = SysTime::ticks();
}

void _SYS_enableCubes(_SYSCubeIDVector cv)
{
    CubeSlot::enableCubes(CubeSlot::truncateVector(cv));
}

void _SYS_disableCubes(_SYSCubeIDVector cv)
{
    CubeSlot::disableCubes(CubeSlot::truncateVector(cv));
}

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf) && CubeSlot::validID(cid))
        CubeSlot::instances[cid].setVideoBuffer(vbuf);
}

void _SYS_loadAssets(_SYSCubeID cid, struct _SYSAssetGroup *group)
{
    if (Runtime::checkUserPointer(group, sizeof *group) && CubeSlot::validID(cid))
        CubeSlot::instances[cid].loadAssets(group);
}

void _SYS_getAccel(_SYSCubeID cid, struct _SYSAccelState *state)
{
    if (Runtime::checkUserPointer(state, sizeof *state) && CubeSlot::validID(cid))
        CubeSlot::instances[cid].getAccelState(state);
}

void _SYS_getNeighbors(_SYSCubeID cid, struct _SYSNeighborState *state) {
    if (Runtime::checkUserPointer(state, sizeof *state) && CubeSlot::validID(cid)) {
        NeighborSlot::instances[cid].getNeighborState(state);
    }
}   

void _SYS_getTilt(_SYSCubeID cid, struct _SYSTiltState *state)
{
    if (Runtime::checkUserPointer(state, sizeof *state) && CubeSlot::validID(cid))
        AccelState::instances[cid].getTiltState(state);
}

void _SYS_getShake(_SYSCubeID cid, _SYS_ShakeState *state)
{
    if (Runtime::checkUserPointer(state, sizeof *state) && CubeSlot::validID(cid))
        AccelState::instances[cid].getShakeState(state);
}


/*
void _SYS_getRawNeighbors(_SYSCubeID cid, uint8_t buf[4])
{
    // XXX: Temporary for testing/demoing
    if (Runtime::checkUserPointer(buf, sizeof buf) && CubeSlot::validID(cid))
        CubeSlot::instances[cid].getRawNeighbors(buf);
}
*/

void _SYS_getRawBatteryV(_SYSCubeID cid, uint16_t *v)
{
    // XXX: Temporary for testing. Master firmware should give cooked battery percentage.
    if (Runtime::checkUserPointer(v, sizeof v) && CubeSlot::validID(cid))
        *v = CubeSlot::instances[cid].getRawBatteryV();
}

void _SYS_getCubeHWID(_SYSCubeID cid, _SYSCubeHWID *hwid)
{
    // XXX: Maybe temporary?
    
    // XXX: Right now this is only guaranteed to be known after asset downloading, since
    //      there is no code yet to explicitly request it (via a flash reset)
    
    if (Runtime::checkUserPointer(hwid, sizeof hwid) && CubeSlot::validID(cid))
        *hwid = CubeSlot::instances[cid].getHWID();
}

void _SYS_vbuf_init(_SYSVideoBuffer *vbuf)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::init(*vbuf);
    }
}

void _SYS_vbuf_lock(_SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        VRAM::lock(*vbuf, addr);
    }
}

void _SYS_vbuf_unlock(_SYSVideoBuffer *vbuf)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::unlock(*vbuf);
    }
}

void _SYS_vbuf_poke(_SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        VRAM::poke(*vbuf, addr, word);
    }
}

void _SYS_vbuf_pokeb(_SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::truncateByteAddr(addr);
        VRAM::pokeb(*vbuf, addr, byte);
    }
}

void _SYS_vbuf_peek(const _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t *word)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        *word = VRAM::peek(*vbuf, addr);
    }
}

void _SYS_vbuf_peekb(const _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t *byte)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        VRAM::truncateByteAddr(addr);
        *byte = VRAM::peekb(*vbuf, addr);
    }
}

void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr,
                    uint16_t word, uint16_t count)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        while (count) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, word);
            count--;
            addr++;
        }
    }
}

void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf) && Runtime::checkUserPointer(src, count << 1)) {
        while (count) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, *src);
            count--;
            addr++;
            src++;
        }
    }
}

void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src,
                      uint16_t offset, uint16_t count)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf) && Runtime::checkUserPointer(src, count << 1)) {
        while (count) {
            uint16_t index = offset + *src;
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, VRAM::index14(index));
            count--;
            addr++;
            src++;
        }
    }
}

void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf)) {
        while (count) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, VRAM::index14(index));
            count--;
            addr++;
            index++;
        }
    }
}

void _SYS_audio_enableChannel(struct _SYSAudioBuffer *buffer)
{
    if (Runtime::checkUserPointer(buffer, sizeof(*buffer))) {
        AudioMixer::instance.enableChannel(buffer);
    }
}

bool _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioHandle *h, _SYSAudioLoopType loop)
{
    if (Runtime::checkUserPointer(mod, sizeof(*mod)) && Runtime::checkUserPointer(h, sizeof(*h))) {
        return AudioMixer::instance.play(mod, h, loop);
    }
    return false;
}

bool _SYS_audio_isPlaying(_SYSAudioHandle h)
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
