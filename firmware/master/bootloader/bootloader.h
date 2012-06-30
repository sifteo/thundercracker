#ifndef _BOOTLOADER_H
#define _BOOTLOADER_H

#include <stdint.h>
#include "aes128.h"

class Bootloader
{
public:
    static const uint8_t VERSION = 1;

    static const unsigned UPDATE_REQUEST_KEY = 0x5A5A5A5A;

    static const uint32_t SIZE = 0x2000;
    static const uint32_t APPLICATION_ADDRESS;
    // 128K total flash, minus the bootloader, and 2 uint32s for CRC and size
    static const uint32_t MAX_APP_SIZE = (128 * 1024) - SIZE - 8;

    enum Command {
        CmdGetVersion,
        CmdWriteMemory,
        CmdWriteFinal,
        CmdResetAddrPtr,
        CmdGetAddrPtr
    };

    static void init();
    static void exec(bool userRequestedUpdate);

    static void onUsbData(const uint8_t *buf, unsigned numBytes);

private:
    static bool manualUpdateRequested();
    static void load();
    static bool eraseMcuFlash();
    static void decryptBlock(uint8_t *plaintext, const uint8_t *cipher);
    static void program(const uint8_t *data, unsigned len);
    static bool mcuFlashIsValid();
    static void cleanup();
    static void jumpToApplication(uint32_t msp, uint32_t resetVector);

    struct Update {
        uint32_t addressPointer;
        uint32_t expandedKey[44];
        uint8_t cipherBuf[AES128::BLOCK_SIZE];
        volatile bool loadInProgress;
    };

    static Update update;
    static bool firstLoad;
};

#endif // _BOOTLOADER_H
