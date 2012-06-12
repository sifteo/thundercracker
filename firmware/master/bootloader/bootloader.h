#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#include <stdint.h>
#include "stm32flash.h"
#include "aes128.h"

class Bootloader
{
public:
    static const uint8_t VERSION = 0;

    static const uint32_t SIZE = 0x2000;
    static const uint32_t APPLICATION_ADDRESS = Stm32Flash::START_ADDR + SIZE;
    static const uint32_t NUM_FLASH_PAGES = SIZE / Stm32Flash::PAGE_SIZE;

    static void init();
    static void exec();

    static void onUsbData(const uint8_t *buf, unsigned numBytes);

private:
    static void load();
    static bool updaterIsEnabled();
    static void disableUpdater();
    static void ensureUpdaterIsEnabled();
    static bool eraseMcuFlash();
    static bool mcuFlashIsValid();
    static void jumpToApplication();

    struct Update {
        uint32_t addressPointer;
        uint32_t expandedKey[44];
        uint8_t cipherBuf[AES128::BLOCK_SIZE];
        volatile bool complete;
    };

    static Update update;
    static bool firstLoad;

    enum Command {
        CmdGetVersion,
        CmdWriteMemory,
        CmdSetAddrPtr,
        CmdGetAddrPtr,
        CmdJump,
        CmdAbort
    };
};

#endif // _BOOTLOADER_H
