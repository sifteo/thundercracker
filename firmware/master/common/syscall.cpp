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

void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t *src, uint16_t count)
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

void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t *src,
                      uint16_t offset, uint16_t count)
{
    if (Runtime::checkUserPointer(vbuf, sizeof *vbuf) && Runtime::checkUserPointer(src, count << 1)) {
        while (count) {
            uint16_t index = offset + *src;
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, ((index << 2) & 0xFE00) | ((index << 1) & 0x00FE));
            count--;
            addr++;
            src++;
        }
    }
}


}  // extern "C"
