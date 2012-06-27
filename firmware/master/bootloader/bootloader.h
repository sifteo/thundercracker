#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#include <stdint.h>
#include "aes128.h"

class Bootloader
{
public:
    static const uint8_t VERSION = 1;

    static const uint32_t SIZE;
    static const uint32_t APPLICATION_ADDRESS;
    static const uint32_t MAX_APP_SIZE = 120 * 1024; // 120K

    enum Command {
        CmdGetVersion,
        CmdWriteMemory,
        CmdWriteDetails,
        CmdResetAddrPtr,
        CmdGetAddrPtr,
        CmdJump,
        CmdAbort
    };

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
    static void jumpToApplication(uint32_t msp, uint32_t resetVector);

    struct Update {
        uint32_t addressPointer;
        uint32_t expandedKey[44];
        uint8_t cipherBuf[AES128::BLOCK_SIZE];
        volatile bool complete;
    };

    static Update update;
    static bool firstLoad;
};

#endif // _BOOTLOADER_H
