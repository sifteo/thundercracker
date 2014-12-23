/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2011 Sifteo, Inc.
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
 * Low-level hardware abstraction layer.
 *
 * Specific to Winbond W29GL032C or similar. Assumptions:
 *
 *   - 8-bit parallel bus
 *   - 32-byte programming buffer
 *   - 64 KB sector erase
 *
 * This HAL assumes we always program the flash 16 words at a time,
 * and we auto-erase just before programming any page that starts
 * at the beginning of an erase-sector.
 *
 * To program:
 *
 *   - Set the address (flash_addr_*)
 *   - Call flash_buffer_begin()
 *   - Call flash_buffer_word() exactly 32 times
 *   - Call flash_buffer_commit()
 *   - The address will have now been incremented by 32 bytes
 *
 * There is no longer any distinction between beginning/ending one
 * buffer vs. beginning/ending a higher-level programming "operation".
 * Calling flash_buffer_commit() always waits for the write to complete.
 * This takes approximately 96us (1500 cycles) on the W29GL032C.
 * Because this delay is so long, any useful work we'd have to do between
 * buffers would be short compared to the time we spend waiting, so it's
 * not worth the code size and complexity.
 */

#include "flash.h"
#include "sensors.h"

uint8_t flash_addr_low;      // Low 7 bits of address, left-justified
uint8_t flash_addr_lat1;     // Middle 7 bits of address, left-justified
uint8_t flash_addr_lat2;     // High 7 bits of address, left-justified
__bit flash_addr_a21;        // Bank selection bit


void flash_strobe();
void flash_buffer_word(void) __naked
{
    /*
     * This is assembly-callable only. Copies a word, pointed to by r1,
     * to the flash programming buffer. Trashes 'a' only.
     *
     * Big endian, to match our LCD byte order.
     *
     * We don't reload the address at all here, we just pre-increment ADDR_PORT
     * twice before each byte. This does generate spurious LCD strobes, which
     * we expect to be ignored after lcd_end_frame().
     *
     * Already expects everything to be set up for writing to the LCD.
     *
     * We have a second entry point, specifically for reuse of the
     * strobe instructions during setup.
     */

    __asm
        inc     r1
        mov     a, @r1

        inc     ADDR_PORT
        inc     ADDR_PORT
        mov     BUS_PORT, a
        mov     CTRL_PORT, #CTRL_FLASH_CMD
        mov     CTRL_PORT, #CTRL_IDLE

        dec     r1
        mov     a, @r1

        inc     ADDR_PORT
        inc     ADDR_PORT
        mov     BUS_PORT, a

_flash_strobe:
        mov     CTRL_PORT, #CTRL_FLASH_CMD
        mov     CTRL_PORT, #CTRL_IDLE
        ret
    __endasm ;
}

#define FLASH_CMD_PREFIX(_addr, _data)          \
    ADDR_PORT = ((_addr) >> 6) & 0xFE;          \
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;    \
    ADDR_PORT = (_addr) << 1;                   \
    BUS_PORT = (_data);                         \
    flash_strobe();


static void flash_prefix_aa_55()
{
    FLASH_CMD_PREFIX(0xAAA, 0xAA);
    FLASH_CMD_PREFIX(0x555, 0x55);
}

static void flash_wait()
{
    /*
     * Wait for a program/erase to complete, by waiting for DQ6 to stop toggling.
     * This function will put the flash in output mode and disable bus drivers.
     *
     * If we see DQ5 go high while DQ6 is still toggling, this means the program
     * or erase operation has timed out! Currently we don't have a good way to handle
     * this, so we'll keep looping until the WDT resets us. To clear the condition,
     * we'd need to pulse RESET on the flash, which we can't do directly- only by
     * turning the 3.3v bus off and back on. It's not worth spending the code space
     * on such a mediocre solution yet.
     */

    BUS_DIR = 0xFF;

    __asm
1$:     mov     CTRL_PORT, #CTRL_IDLE           ; Read cycle 1
        mov     CTRL_PORT, #CTRL_FLASH_OUT
        mov     a, BUS_PORT

        mov     CTRL_PORT, #CTRL_IDLE           ; Read cycle 2
        mov     CTRL_PORT, #CTRL_FLASH_OUT
        cjne    a, BUS_PORT, 1$
    __endasm;

    CTRL_PORT = CTRL_IDLE;
}

void flash_buffer_begin(void)
{
    /*
     * Set up pre-erase addressing.
     *
     * If we do need to do an erase, that will require changing LAT1 and the
     * low byte, but we can go ahead and set up LAT2 and A21 now.
     *
     * Callers expect us not to clobber any of R0-R7 or DPTR!
     */

    // Set up A21
    i2c_a21_target = flash_addr_a21;
    i2c_a21_wait();

    // We can keep LAT2 loaded, since the prefix sequences don't use it.
    // Also, set up the bus direction and make sure the flash is otherwise idle.
    ADDR_PORT = flash_addr_lat2;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT2;
    BUS_DIR = 0;

    /*
     * If we're at a sector boundary (64 KB, 512 tiles, x:xxxxx00:0000000:0000000)
     * If so, erase the sector and wait for the erase to finish.
     */
    if (flash_addr_low == 0 && flash_addr_lat1 == 0 && (flash_addr_lat2 & 7) == 0) {

        // Common unlock prefix for all erase ops
        flash_prefix_aa_55();
        FLASH_CMD_PREFIX(0xAAA, 0x80);
        flash_prefix_aa_55();

        // Sector erase. (LAT2 already contains sector addr, low bits are don't-care)
        BUS_PORT = 0x30;
        flash_strobe();

        // Wait for completion
        flash_wait();
        BUS_DIR = 0;
    }

    flash_prefix_aa_55();       // Unlock
    BUS_PORT = 0x25;            // Buffer program
    flash_strobe();
    BUS_PORT = 31;              // Writing 32 bytes
    flash_strobe();

    /*
     * Now that they won't be clobbered by commands, set up the remainder
     * of the address. LAT1 stays constant throughout the block, and LOW lives
     * in ADDR_PORT from here on.
     *
     * We subtract two here to undo the preincrement on our first byte.
     */

    ADDR_PORT = flash_addr_lat1;
    CTRL_PORT = CTRL_IDLE | CTRL_FLASH_LAT1;
    ADDR_PORT = flash_addr_low - 2;
}

void flash_buffer_commit(void)
{
    /*
     * Callers expect us not to clobber any of R0-R7 or DPTR!
     */
    
    // Confirmation byte. This starts the program operation.
    BUS_PORT = 0x29;
    flash_strobe();

    /*
     * Anything we do here is effectively free, as far as CPU time goes.
     * Unfortunately there isn't much we can do on the main thread
     * without flash, unless we feel like rendering part of a scanline
     * and we don't need flash to do that.
     *
     * So.. for now, all we have to do is update our address counter.
     */
     
    __asm
        mov     a, _flash_addr_low      ; 32 bytes
        add     a, #64
        mov     _flash_addr_low, a
        jnz     1$

        mov     a, _flash_addr_lat1
        add     a, #2
        mov     _flash_addr_lat1, a
        jnz     1$

        inc     _flash_addr_lat2
        inc     _flash_addr_lat2
1$:
    __endasm ;

    // Wait for the program to finish, and release the bus.
    flash_wait();
}
