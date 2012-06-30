/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "panic.h"
#include "vram.h"
#include "svmmemory.h"
#include "cubeslots.h"
#include "cube.h"
#include "systime.h"
#include "led.h"
#include "tasks.h"
#include "homebutton.h"


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
     *
     * If the paint can't be completed in a fixed amount of time,
     * we abort. This ensures that paint() returns even if the
     * indicated cube is no longer reachable.
     */

    dumpScreenToUART();

    SysTime::Ticks deadline = SysTime::ticks() + SysTime::sTicks(1);

    CubeSlot &slot = CubeSlots::instances[cube];
    avb->vbuf.vram.flags = _SYS_VF_CONTINUOUS;

    CubeSlots::connectCubes(slot.bit());
    CubeSlots::enableCubes(slot.bit());

    VRAM::init(avb->vbuf);
    VRAM::unlock(avb->vbuf);
    slot.setVideoBuffer(&avb->vbuf);

    // Wait for the radio transmission to finish
    while (avb->vbuf.cm16 != 0 && SysTime::ticks() < deadline)
        Tasks::idle();

    // Wait for the cube to draw, twice. It may have been partway
    // through a frame when the radio transmission finished.
    for (unsigned i = 0; i < 2; i++) {
        uint8_t baseline = slot.getLastFrameACK();
        while (slot.getLastFrameACK() == baseline && SysTime::ticks() < deadline)
            Tasks::idle();
    }

    // Turn off rendering
    VRAM::pokeb(avb->vbuf, _SYS_VA_FLAGS, 0);
    VRAM::unlock(avb->vbuf);

    // Wait for the radio transmission to finish
    while (avb->vbuf.cm16 != 0&& SysTime::ticks() < deadline)
        Tasks::idle();

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

void PanicMessenger::dumpScreenToUART()
{
    //      0123456789ABCDEF
    UART(("+---- PANIC! ----+\r\n"));

    unsigned addr = 0;
    for (unsigned y = 0; y != 16; y++) {
        UART(("|"));
        for (unsigned x = 0; x != 16; x++) {

            // Read back from BG0
            unsigned index = avb->vbuf.vram.bg0_tiles[addr++];
            index = _SYS_INVERSE_TILE77(index);

            // Any non-text tiles are drawn as '.'
            if (index <= unsigned('~' - ' '))
                index += ' ';
            else
                index = '.';

            // Interpret 32-bit word as char plus NUL terminator
            UART((reinterpret_cast<char*>(&index)));
        }
        UART(("|\r\n"));
        addr += 2;
    }

    //      0123456789ABCDEF
    UART(("+----------------+\r\n"));
}

void PanicMessenger::haltForever()
{
    LOG(("PANIC: System halted!\n"));
    UART(("PANIC: System halted!\r\n"));

    while (1)
        animateLED();
}

void PanicMessenger::haltUntilButton()
{
    LOG(("PANIC: Waiting for button press\n"));
    UART(("PANIC: Waiting for button press\r\n"));

    // Wait for press
    while (!HomeButton::isPressed())
        animateLED();

    // Wait for release
    while (HomeButton::isPressed())
        animateLED();
}

void PanicMessenger::animateLED()
{
    static const uint8_t pattern[] = {
        // OH NO!!!!
        LED::RED, LED::GREEN, LED::OFF, LED::OFF,
    };

    STATIC_ASSERT(arraysize(pattern) == 4);
    LED::set(LED::Color(pattern[(SysTime::ticks() >> 26) & 3]));

    Tasks::idle();
}
