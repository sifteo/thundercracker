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

#ifndef STM32FLASH_H
#define STM32FLASH_H

#include <stdint.h>
#include "hardware.h"

class Stm32Flash
{
public:
    static const uint16_t RDP_DISABLE_KEY = 0x00A5;
    static const uint32_t KEY1 = 0x45670123;
    static const uint32_t KEY2 = 0xCDEF89AB;

    // connectivity line specific values
    static const unsigned PAGE_SIZE = 2 * 1024;
    static const unsigned NUM_PAGES = 64;

    static const uint32_t START_ADDR = 0x8000000;
    static const uint32_t END_ADDR = START_ADDR + (NUM_PAGES * PAGE_SIZE);

    static inline void unlock() {
        FLASH.KEYR = KEY1;
        FLASH.KEYR = KEY2;
    }

    static inline void unlockOptionBytes() {
        FLASH.OPTKEYR = KEY1;
        FLASH.OPTKEYR = KEY2;
    }

    /*
     * Programming:
     *  - beginProgramming()
     *  - one or more calls to programHalfWord()
     *  - endProgramming()
     */
    static inline void beginProgramming() {
        FLASH.CR |= 0x1;
    }

    static bool programHalfWord(uint16_t halfword, uint32_t address);

    static inline void endProgramming() {
        waitForPreviousOperation();
        FLASH.CR &= ~0x1;
    }

    /*
     * Erasing:
     *  - beginErasing()
     *  - one or more calls to erasePage()
     *  - endErasing()
     */
    static inline void beginErasing() {
        FLASH.CR |= (1 << 1);
    }

    static bool erasePage(uint32_t address);

    static inline void endErasing() {
        waitForPreviousOperation();
        FLASH.CR &= ~(1 << 1);
    }

    static bool readOutProtectionIsEnabled() {
        return (FLASH.OBR & (1 << 1)) != 0;
    }

    enum OptionByte {
        OptionRDP,
        OptionUSER,
        OptionDATA0,
        OptionDATA1,
        OptionWRP0,
        OptionWRP1,
        OptionWRP2,
        OptionWRP3,
    };

    static bool eraseOptionBytes(bool enableRDP = false);
    static bool setOptionByte(OptionByte ob, uint16_t value);

private:
    enum WaitStatus {
        WaitOk,
        WaitError
    };

    static WaitStatus waitForPreviousOperation();
};

#endif // STM32FLASH_H
