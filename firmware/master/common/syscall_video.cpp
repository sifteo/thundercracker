/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for video buffer manipulation.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "vram.h"

extern "C" {

void _SYS_vbuf_init(_SYSVideoBuffer *vbuf)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::init(*vbuf);
    }
}

void _SYS_vbuf_lock(_SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        VRAM::lock(*vbuf, addr);
    }
}

void _SYS_vbuf_unlock(_SYSVideoBuffer *vbuf)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::unlock(*vbuf);
    }
}

void _SYS_vbuf_poke(_SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        VRAM::poke(*vbuf, addr, word);
    }
}

void _SYS_vbuf_pokeb(_SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateByteAddr(addr);
        VRAM::pokeb(*vbuf, addr, byte);
    }
}

uint32_t _SYS_vbuf_peek(const _SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        return VRAM::peek(*vbuf, addr);
    }
    return 0;
}

uint32_t _SYS_vbuf_peekb(const _SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateByteAddr(addr);
        return VRAM::peekb(*vbuf, addr);
    }
    return 0;
}

void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr,
                    uint16_t word, uint16_t count)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
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
    if (!SvmMemory::mapRAM(vbuf, sizeof *vbuf))
        return;

    FlashBlockRef ref;
    uint32_t bytes = SvmMemory::arraySize(sizeof *src, count);
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    SvmMemory::PhysAddr srcPA;

    while (bytes) {
        uint32_t chunk = bytes;
        if (!SvmMemory::mapROData(ref, srcVA, chunk, srcPA))
            return;

        srcVA += chunk;
        bytes -= chunk;

        while (chunk) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, *reinterpret_cast<uint16_t*>(srcPA));
            addr++;

            chunk -= sizeof(uint16_t);
            srcPA += sizeof(uint16_t);
        }
    }
}

void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src,
                      uint16_t offset, uint16_t count)
{
    if (!SvmMemory::mapRAM(vbuf, sizeof *vbuf))
        return;

    FlashBlockRef ref;
    uint32_t bytes = SvmMemory::arraySize(sizeof *src, count);
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    SvmMemory::PhysAddr srcPA;

    ASSERT((bytes & 1) == 0);
    while (bytes) {
        uint32_t chunk = bytes;
        if (!SvmMemory::mapROData(ref, srcVA, chunk, srcPA))
            return;

        ASSERT((chunk & 1) == 0);
        srcVA += chunk;
        bytes -= chunk;

        while (chunk) {
            uint16_t index = offset + *reinterpret_cast<uint16_t*>(srcPA);

            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, _SYS_TILE77(index));
            addr++;

            chunk -= sizeof(uint16_t);
            srcPA += sizeof(uint16_t);
        }
    }
}

void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        while (count) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, _SYS_TILE77(index));
            count--;
            addr++;
            index++;
        }
    }
}

void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count,
                     uint16_t lines, uint16_t src_stride, uint16_t addr_stride)
{
    /*
     * Shortcut for a rectangular blit, comprised of multiple calls to SYS_vbuf_writei.
     * We save the pointer validation for writei, since we want to do it per-scanline anyway.
     */

    while (lines--) {
        _SYS_vbuf_writei(vbuf, addr, src, offset, count);
        src += src_stride;
        addr += addr_stride;
    }
}

void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height)
{
    // Address validation occurs after these calculations, in _SYS_vbuf_poke.
    // Sprite ID validation is implicit.

    uint8_t xb = -(int)width;
    uint8_t yb = -(int)height;
    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                     sizeof(_SYSSpriteInfo)/2 * id );

    _SYS_vbuf_poke(vbuf, addr, word);
}

void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y)
{
    // Address validation occurs after these calculations, in _SYS_vbuf_poke.
    // Sprite ID validation is implicit.

    uint8_t xb = -x;
    uint8_t yb = -y;
    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id );

    _SYS_vbuf_poke(vbuf, addr, word);
}

}  // extern "C"
