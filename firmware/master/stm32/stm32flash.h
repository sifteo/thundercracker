#ifndef STM32FLASH_H
#define STM32FLASH_H

#include <stdint.h>
#include "hardware.h"

class Stm32Flash
{
public:
    static const uint16_t READOUT_PROTECT_KEY = 0x00A5;
    static const uint32_t KEY1 = 0x45670123;
    static const uint32_t KEY2 = 0xCDEF89AB;

    // connectivity line specific values
    static const unsigned PAGE_SIZE = 2 * 1024;
    static const unsigned NUM_PAGES = 64;

    static const uint32_t START_ADDR = 0x8000000;
    static const uint32_t END_ADDR = START_ADDR + (NUM_PAGES * PAGE_SIZE);

    static void unlock() {
        FLASH.KEYR = KEY1;
        FLASH.KEYR = KEY2;
    }

    /*
     * Programming:
     *  - beginProgramming()
     *  - one or more calls to programHalfWord()
     *  - endProgramming()
     */
    static void beginProgramming() {
        FLASH.CR |= 0x1;
    }

    static bool programHalfWord(uint16_t halfword, uint32_t address);

    static void endProgramming() {
        waitForPreviousOperation();
        FLASH.CR &= ~0x1;
    }

    /*
     * Erasing:
     *  - beginErasing()
     *  - one or more calls to erasePage()
     *  - endErasing()
     */
    static void beginErasing() {
        FLASH.CR |= (1 << 1);
    }

    static bool erasePage(uint32_t address);

    static void endErasing() {
        waitForPreviousOperation();
        FLASH.CR &= ~(1 << 1);
    }

    static bool readOutProtectionIsEnabled() {
        return (FLASH.OBR & (1 << 1)) != 0;
    }

    struct OptionByte {
        bool program;
        uint16_t value;
    };

    static const unsigned NUM_OPTION_BYTES = 8;
    static bool setOptionBytes(OptionByte *optionBytes);

private:
    enum WaitStatus {
        WaitOk,
        WaitError
    };

    static WaitStatus waitForPreviousOperation();
};

#endif // STM32FLASH_H
