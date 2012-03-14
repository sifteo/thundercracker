/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Flash hardware abstraction layer.
 *
 * Currently this is written for SST flashes, specifically the
 * SST39VF1681. But we expect to have to modify this to suit whatever
 * commodity flash memory we source for the product.

 NOTE: this has been updated to also support the Winbond W29GL032C.
 Relevant changes:
  - flash_addr_lat2 increments by 2 instead of 4 for adjacent sector addresses for erase
  - lat1 and lat2 lines are swapped on buddy cube rev 1 - this is accounted for in hardware.h
  - due to a routing error on buddy cube rev 1, we must use WORD_MODE instead of
    BYTE_MODE, and your board must be modded accordingly, since the BYTE select
    pin would normally be tied low.

 TODO: the winbond part also provides a bulk programming mode that would allow
        us to optimize by  only sending the unlock sequence for each
        batch of bytes that we want to program, rather than for each byte as we do now.
 */

#include "flash.h"
#include "radio.h"

uint8_t flash_addr_low;
uint8_t flash_addr_lat1;
uint8_t flash_addr_lat2;

/*
 * We can poll in a much tighter loop (and therefore exit the polling
 * with lower latency) if we use Data# polling as opposed to toggle
 * polling, since we don't need to pulse the OE pin. This is probably
 * also rather lower power consumption, since we aren't toggling an
 * external pin in a tight loop!
 *
 * This polling is based on examining the most significant bit of the
 * data byte. While busy, it will be the inverse of the true data.
 *
 * XXX: This can hang forever if we try to program a '0' bit to '1'!
 *      We probably want to detect this and other flash errors via some
 *      kind of watchdog?
 */

static __bit flash_poll_data;  // What data bit are we expecting?

/*
 * Output a constant unlock prefix. Only the low 12 bits of address
 * matter, so we don't have to set LAT2. We *must* keep the address
 * and data stable for the entire time that WE is asserted.
 */

#define FLASH_CMD_STROBE()                      \
    CTRL_PORT = CTRL_FLASH_CMD;                 \
    CTRL_PORT = CTRL_IDLE;

#define FLASH_CMD_PREFIX(_addr, _dat)           \
    ADDR_PORT = ((_addr) >> 6) & 0xFE;          \
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;    \
    ADDR_PORT = (_addr) << 1;                   \
    BUS_PORT = (_dat);                          \
    FLASH_CMD_STROBE()

#define FLASH_OUT()                             \
    BUS_DIR = 0xFF;                             \
    CTRL_PORT = CTRL_FLASH_OUT;

/*
    Byte mode vs Word Mode.
    Normally we'd like to program a byte a time, but buddy cube rev 1
    has a routing mistake that means we must use word mode - but we're still only
    feeding it a byte at a time, so we're essentially discarding the most
    significant byte and only using half our available storage :(

    Presume this will be removed for subsequent designs
*/
#define BYTE_MODE   1
#define WORD_MODE   2

#ifndef FLASH_PROGRAM_MODE
#define FLASH_PROGRAM_MODE WORD_MODE
#endif

static void flash_prefix_aa_55()
{
#if FLASH_PROGRAM_MODE == BYTE_MODE
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
#elif FLASH_PROGRAM_MODE == WORD_MODE
    FLASH_CMD_PREFIX(0x555, 0xAA);
    FLASH_CMD_PREFIX(0x2AA, 0x55);
#endif
}

static void flash_write_unlock()
{
    // Write unlock prefix
    flash_prefix_aa_55();
#if FLASH_PROGRAM_MODE == BYTE_MODE
    FLASH_CMD_PREFIX(0xAAA, 0xA0);
#elif FLASH_PROGRAM_MODE == WORD_MODE
    FLASH_CMD_PREFIX(0x555, 0xA0);
#endif
}
    
void flash_autoerase(void)
{
    /*
     * Erase one sector (64Kb) at the current address, if and only if
     * we're pointed to the first byte of a flash block. Guaranteed
     * to be called at tile boundaries only.
     *
     * This is invoked prior to every decompressed tile, so that we
     * automatically erase blocks as we reach them during decompression.
     *
     * Erase operations are self-contained: we do all address setup
     * (including lat2) and we wait for the erase to finish.
     */

    // Check sector alignment
    if (flash_addr_lat1)
        return;
    if (flash_addr_lat2 & 7)
        return;

    CTRL_PORT = CTRL_IDLE;
    BUS_DIR = 0;

    // XXX: Critical section only needed for WORD_MODE hack below.
    radio_critical_section({

        // Common unlock prefix for all erase ops
        flash_prefix_aa_55();
        #if FLASH_PROGRAM_MODE == BYTE_MODE
        FLASH_CMD_PREFIX(0xAAA, 0x80);
        #elif FLASH_PROGRAM_MODE == WORD_MODE
        FLASH_CMD_PREFIX(0x555, 0x80);
        #endif
        flash_prefix_aa_55();

        // Sector erase. (Low bits of address are Don't Care)
        ADDR_PORT = flash_addr_lat2;
        CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
        ADDR_PORT = 0;
        BUS_PORT = 0x30;
        FLASH_CMD_STROBE();

        #if FLASH_PROGRAM_MODE == WORD_MODE
            // XXX - we need to make up for our WORD/BYTE screwiness and erase
            //       an extra sector. In WORD mode, the device sectors are
            //       64 kbytes or 32 kwords, so they appear as 32 kB to us.
            //
            // NOTE: after the first sector erase command has been received,
            // subsequent commands may be received within a 50us timeout
            // without having to send another unlock sequence.
            // ie, don't put anything else in this loop :)
            // longer term, master should only be telling us exactly
            // what it wants us to erase

            ADDR_PORT = flash_addr_lat2 + 4;
            CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
            ADDR_PORT = 0;
            BUS_PORT = 0x30;
            FLASH_CMD_STROBE();
        #endif
    )};

    // Wait for completion
    FLASH_OUT();
    __asm  2$:  jnb     BUS_PORT.7, 2$  __endasm;

    flash_program_start();
}

void flash_program_start(void)
{
    /*
     * Perform infrequent initialization steps that only need to occur
     * before a batch of program operations, and after any intervening
     * flash reads.
     */

    // We can keep LAT2 loaded, since the prefix sequences don't use it.
    ADDR_PORT = flash_addr_lat2;
    CTRL_PORT = CTRL_FLASH_OUT | CTRL_FLASH_LAT2;

    // Set flash_poll_data to whatever data is at the current address,
    // so we don't have to special-case the first program operation.

    __asm
        mov     c, BUS_PORT.7
        mov     _flash_poll_data, c
    __endasm ;
}    

void flash_program_end(void)
{
    /*
     * The counterpart to flash_program_start(). Wait for the last
     * write to finish, and release the bus.
     */
     
    __asm
        jnb     _flash_poll_data, 2$
3$:     jnb     BUS_PORT.7, 3$
        sjmp    1$
2$:     jb      BUS_PORT.7, 2$
1$:
    __endasm ;
    
    CTRL_PORT = CTRL_IDLE;
}

void flash_program_word(uint16_t dat) __naked
{
    /*
     * Program two bytes, at any aligned address. Increment the
     * address. We use Big Endian here, to match the LCD's byte order.
     *
     * The run length must be at least 1.
     * The bytes MUST have been erased first.
     *
     * This routine is highly performance critical itself, plus we
     * need to be careful about its impact on the flash decoder code.
     * This routine is written without any temporary registers, and we
     * use "#pragma callee_saves" to allow SDCC to notice this and
     * omit unnecessary save/restore code.
     */

    dat = dat;

    /*
     * Low byte
     */

    // Wait on the previous word-write
    __asm
        jnb     _flash_poll_data, 2$
3$:     jnb     BUS_PORT.7, 3$
        sjmp    1$
2$:     jb      BUS_PORT.7, 2$
1$:
    __endasm ;

    CTRL_PORT = CTRL_IDLE;
    BUS_DIR = 0;

    flash_write_unlock();

    // Write byte
    __asm
        mov     ADDR_PORT, _flash_addr_lat1
        mov     CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT1)
        mov     ADDR_PORT, _flash_addr_low
        mov     BUS_PORT, DPH
    __endasm ;
    FLASH_CMD_STROBE();
    FLASH_OUT();

    /*
     * We can do other useful work in-between bytes. This is
     * effectively free, since we'd be waiting anyway.
     */

    flash_addr_low += 2;

    // Calculate the next flash_poll_data flag.
    __asm
        mov     a, DPL
        rlc     a
        mov     _flash_poll_data, c
    __endasm ;

    /*
     * High byte
     */

   // Wait for the low byte to finish
    __asm
        mov     a, DPH
        rlc     a
        jnc     5$

6$:     jnb     BUS_PORT.7, 6$
        sjmp    7$

5$:     jb      BUS_PORT.7, 5$
7$:
    __endasm ;

    CTRL_PORT = CTRL_IDLE;
    BUS_DIR = 0;

    flash_write_unlock();

    // Write data byte, without any temporary registers
    __asm
        mov     ADDR_PORT, _flash_addr_lat1
        mov     CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT1)
        mov     ADDR_PORT, _flash_addr_low
        mov     BUS_PORT, DPL
    __endasm ;
    FLASH_CMD_STROBE();
    FLASH_OUT();

    // Increment flash_addr on our way out, without any temporaries
    __asm
        ; Common case, no overflow
        mov     a, _flash_addr_low
        add     a, #2
        mov     _flash_addr_low, a
        jz      11$
        ret

        ; Low byte overflow 
11$:
        mov     a, _flash_addr_lat1
        add     a, #2
        mov     _flash_addr_lat1, a
        jz      12$
        ret

        ; Lat1 overflow
12$:
        mov     a, _flash_addr_lat2
        add     a, #2
        mov     _flash_addr_lat2, a

        ; Since we keep lat2 loaded into the latch, we need to at some point
        ; reload the latch before the next write. But we must not touch the
        ; latch while the flash is busy! In the interest of keeping the common
        ; case fast, just wait for the current write to finish then update lat2.

        jnb     _flash_poll_data, 10$
8$:     jnb     BUS_PORT.7, 8$
        sjmp    9$
10$:    jb      BUS_PORT.7, 10$
9$:

        mov     ADDR_PORT, a
        mov     CTRL_PORT, #(CTRL_IDLE | CTRL_FLASH_LAT2)

        mov     c, BUS_PORT.7
        mov     _flash_poll_data, c

        ret

    __endasm ;
}
