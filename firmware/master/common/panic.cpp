/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "panic.h"
#include "vram.h"
#include "svmmemory.h"
#include "cubeslots.h"
#include "cube.h"


void PanicMessenger::init(SvmMemory::VirtAddr vbufVA)
{
    // Initialize, with vbuf memory stolen from userspace at the specified address.

    SvmMemory::PhysAddr vbufPA;
    bool success = SvmMemory::mapRAM(vbufVA, sizeof(_SYSAttachedVideoBuffer), vbufPA);
    avb = success ? reinterpret_cast<_SYSAttachedVideoBuffer*>(vbufPA) : 0;
    ASSERT(avb != 0);
    erase();
}

void PanicMessenger::erase()
{
    memset(avb, 0, sizeof *avb);
    avb->vbuf.vram.num_lines = 128;
    avb->vbuf.vram.mode = _SYS_VM_BG0_ROM;
}

void PanicMessenger::paint(_SYSCubeID cube)
{
    /*
     * _Synchronously_ paint this cube.
     * We'll connect and enable it if needed.
     *
     * This is really heavy-handed: All of VRAM is redrawn, we
     * use continuous rendering mode (to avoid needing to know the
     * previous toggle state) and we synchonously wait for everything
     * to finish.
     *
     * The video buffer is only attached during this call.
     *
     * We try not to invoke any Task handlers, as would be the case
     * if we used higher-level paint primitives.
     */

    CubeSlot &slot = CubeSlots::instances[cube];
    avb->vbuf.vram.flags = _SYS_VF_CONTINUOUS;

    CubeSlots::connectCubes(slot.bit());
    CubeSlots::enableCubes(slot.bit());

    VRAM::init(avb->vbuf);
    VRAM::unlock(avb->vbuf);
    slot.setVideoBuffer(&avb->vbuf);

    // Wait for the radio transmission to finish
    while (avb->vbuf.cm16 != 0) {
        Atomic::Barrier();
        Radio::halt();
    }

    // Wait for the cube to draw, twice. It may have been partway
    // through a frame when the radio transmission finished.
    for (unsigned i = 0; i < 2; i++) {
        uint8_t baseline = slot.getLastFrameACK();
        while (slot.getLastFrameACK() == baseline) {
            Atomic::Barrier();
            Radio::halt();
        }
    }

    // Turn off rendering
    VRAM::pokeb(avb->vbuf, _SYS_VA_FLAGS, 0);
    VRAM::unlock(avb->vbuf);

    // Wait for the radio transmission to finish
    while (avb->vbuf.cm16 != 0) {
        Atomic::Barrier();
        Radio::halt();
    }

    // Stop using this VideoBuffer
    slot.setVideoBuffer(NULL);
}

PanicMessenger &PanicMessenger::operator<< (char c)
{
    unsigned index = c - ' ';
    avb->vbuf.vram.bg0_tiles[addr++] = _SYS_TILE77(index);
    return *this;
}

PanicMessenger &PanicMessenger::operator<< (const char *str)
{
    while (char c = *(str++))
        *this << c;
    return *this;
}

PanicMessenger &PanicMessenger::operator<< (uint8_t byte)
{
    const char *digits = "0123456789ABCDEF";
    *this << digits[byte >> 4] << digits[byte & 0xf];
    return *this;
}

PanicMessenger &PanicMessenger::operator<< (uint32_t word)
{
    *this << uint8_t(word >> 24) << uint8_t(word >> 16)
          << uint8_t(word >> 8) << uint8_t(word);
    return *this;
}
