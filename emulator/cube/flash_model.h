/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo prototype simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * This header holds all of the model-specific details for our flash
 * memory.  The intention is for flash.c to be as model-agnostic as
 * possible, so it's easier to switch to other NOR parts in the
 * future.
 *
 * Currently this is based on the SST39VF1681, but this likely isn't
 * what we'll actually be using in the final design.
 *
 * Our current requirements:
 *
 *  - NOR flash, 2Mx8 (16 megabits)
 *
 *  - Low power (Many memories have an automatic standby feature, which gives them
 *    fairly low power usage when we're clocking them slowly. Our actual clock rate
 *    will be 2.67 MHz peak.)
 *
 *  - Fast program times. The faster we can get the better, this will directly
 *    impact asset download times.
 *
 *  - Fast erase times. This is very important- we'd like to stream assets in the
 *    background, but during an erase we can't refresh the screen at all until the
 *    erase finishes. This is clearly very bad for interactivity, so flashes with
 *    erase times in the tens of milliseconds are much better than flashes that
 *    take half a second or longer to erase.
 */

#include <stdint.h>

#ifndef _FLASH_MODEL_H
#define _FLASH_MODEL_H

struct FlashModel {
    /*
     * Flash geometry
     */

    static const unsigned SIZE         = 2 * 1024 * 1024;
    static const unsigned BLOCK_SIZE   = 64 * 1024;
    static const unsigned SECTOR_SIZE  = 4 * 1024;

    /*
     * Structure of the status byte, returned while the flash is busy.
     */
    
    static const uint8_t STATUS_DATA_INV      = 0x80;    // Inverted bit of the written data
    static const uint8_t STATUS_TOGGLE        = 0x40;    // Toggles during any busy state
    static const uint8_t STATUS_ERASE_TOGGLE  = 0x04;    // Toggles only during erase
    
    /*
     * Program/erase timing.
     *
     * Currently these are based on "typical" datasheet values. We may
     * want to test with maximum allowed values too.
     */

    static const unsigned PROGRAM_TIME_US       = 7;
    static const unsigned ERASE_SECTOR_TIME_US  = 18000;
    static const unsigned ERASE_BLOCK_TIME_US   = 18000;
    static const unsigned ERASE_CHIP_TIME_US    = 40000;
    
    /*
     * The subset of commands we emulate.
     *
     * Each command is expressed as a pattern which matches against a
     * buffer of the last CMD_LENGTH write cycles.
     */

    static const unsigned CMD_LENGTH = 6;

    struct command_sequence {
        uint16_t addr_mask;
        uint16_t addr;
        uint8_t data_mask;
        uint8_t data;
    };
    
    static const struct command_sequence cmd_byte_program[] = {
        { 0x000, 0x000, 0x00, 0x00 },
        { 0x000, 0x000, 0x00, 0x00 },
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0xFFF, 0xAAA, 0xFF, 0xA0 },
        { 0x000, 0x000, 0x00, 0x00 },
    };

    static const struct command_sequence cmd_sector_erase[] = {
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0xFFF, 0xAAA, 0xFF, 0x80 },
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0x000, 0x000, 0xFF, 0x50 },
    };

    static const struct command_sequence cmd_block_erase[] = {
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0xFFF, 0xAAA, 0xFF, 0x80 },
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0x000, 0x000, 0xFF, 0x30 },
    };

    static const struct command_sequence cmd_chip_erase[] = {
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0xFFF, 0xAAA, 0xFF, 0x80 },
        { 0xFFF, 0xAAA, 0xFF, 0xAA },
        { 0xFFF, 0x555, 0xFF, 0x55 },
        { 0xFFF, 0xAAA, 0xFF, 0x10 },
    };
};

#endif

