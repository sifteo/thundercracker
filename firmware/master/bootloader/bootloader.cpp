
#include "bootloader.h"
#include "flash_device.h"
#include "stm32flash.h"
#include "crc.h"
#include "tasks.h"
#include "usbprotocol.h"
#include "usb/usbdevice.h"
#include "aes128.h"

#include "string.h"

Bootloader::Update Bootloader::update;

Bootloader::Bootloader()
{
    
}

void Bootloader::init()
{
    FlashDevice::init();
    Crc32::init();
    Tasks::init();
}

/*
 * High level program flow of the bootloader.
 */
void Bootloader::exec()
{
    if (updaterIsEnabled())
        load();

    while (!mcuFlashIsValid())
        load();

    if (updaterIsEnabled())
        disableUpdater();

    jumpToApplication();
}

/*
 * Load a new firmware image from our host over USB.
 *  - erase enough room in Stm32Flash for the new image
 *  - program incrementally as we receive data over USB
 *  -
 */
void Bootloader::load()
{
    // XXX: check if USB is already enabled (ie, this is not our first attempt)
    UsbDevice::init();

    Stm32Flash::unlock();
    if (!eraseMcuFlash())
        return;

    // initialize decryption
    // XXX: select actual values
    const uint32_t key[4] = { 0x2b7e1516, 0x28aed2a6, 0xabf71588, 0x09cf4f3c };
    const uint32_t iv[4] = { 0x2b7e1516, 0x28aed2a6, 0xabf71588, 0x09cf4f3c };
    AES128::expandKey(update.expandedKey, key);
    memcpy(update.cipherBuf, iv, AES128::BLOCK_SIZE);

    // initialize update state, default to beginning of application flash
    update.complete = false;
    update.addressPointer = APPLICATION_ADDRESS;

    while (!update.complete)
        Tasks::work();

    // success is determined by the validation of MCU flash
}

/*
 * Data has arrived from the host. Execute commands accordingly.
 */
void Bootloader::onUsbData(const uint8_t *buf, unsigned numBytes)
{
    switch (buf[0]) {
    case CmdGetVersion: {
        const uint8_t response[] = { buf[0], VERSION };
        UsbDevice::write(response, sizeof response);
        break;
    }

    case CmdWriteMemory: {
        // packets should be block aligned, after the cmd byte
        if ((numBytes - 1) % AES128::BLOCK_SIZE != 0)
            return;

        uint8_t plaintext[AES128::BLOCK_SIZE];
        const uint8_t *cipherIn = buf + 1;

        AES128::encryptBlock(plaintext, update.cipherBuf, update.expandedKey);
        AES128::xorBlock(plaintext, cipherIn);

        const uint16_t *p = reinterpret_cast<const uint16_t*>(plaintext);
        for (unsigned i = 0; i < AES128::BLOCK_SIZE / 2; ++i) {
            Stm32Flash::programHalfWord(*p, update.addressPointer);
            update.addressPointer += 2;
            p++;
        }
        break;
    }

    case CmdSetAddrPtr: {
        uint32_t ptr = *reinterpret_cast<const uint32_t*>(buf + 1);
        if ((APPLICATION_ADDRESS <= ptr) && (ptr < Stm32Flash::END_ADDR))
            update.addressPointer = ptr;
        break;
    }

    case CmdGetAddrPtr: {
        uint8_t response[5] = { buf[0] };
        memcpy(response + 1, &update.addressPointer, sizeof update.addressPointer);
        UsbDevice::write(response, sizeof response);
        break;
    }

    case CmdJump:
    case CmdAbort:
        update.complete = true;
        break;
    }
}

/*
 * Erase the user portion of MCU flash - everything beyond the bootloader.
 */
bool Bootloader::eraseMcuFlash()
{
    Stm32Flash::beginErasing();

    unsigned a = APPLICATION_ADDRESS;
    unsigned appFlashPages = Stm32Flash::NUM_PAGES - NUM_FLASH_PAGES;
    unsigned end = a + (Stm32Flash::PAGE_SIZE * appFlashPages);

    while (a < end) {
        if (!Stm32Flash::erasePage(a))
            return false;
        a += Stm32Flash::PAGE_SIZE;
    }

    Stm32Flash::endErasing();

    return true;
}

/*
 * Returns whether the updater functionality is enabled or not.
 * This is a persistent setting that can only be reset once we have
 * successfully installed a new update.
 */
bool Bootloader::updaterIsEnabled()
{
    return (FLASH_OB.Data0 & 0x1) == 0x1;
}

/*
 * Disable the persistent updater bit.
 * Indicates that an update was successfully applied, and user code is ready to run.
 */
void Bootloader::disableUpdater()
{
    // enable readout protect, retain USER byte, and clear DATA bytes
    Stm32Flash::OptionByte optionBytes[Stm32Flash::NUM_OPTION_BYTES];
    memset(optionBytes, 0, sizeof optionBytes);

    optionBytes[0].program = true;  // RDP is index 0, enable it.
    optionBytes[0].value = 0;

    optionBytes[3].program = true;  // DATA0 is index 3, clear it
    optionBytes[3].value = 0;

    Stm32Flash::setOptionBytes(optionBytes);
}

/*
 * Validates the CRC of the contents of MCU flash.
 */
bool Bootloader::mcuFlashIsValid()
{
    uint32_t crc, imgsize;
    // TODO: determine how we store these
    crc = imgsize = 0;

    Crc32::reset();
    uint32_t *address = reinterpret_cast<uint32_t*>(APPLICATION_ADDRESS);
    const uint32_t *end = address + imgsize;

    while (address < end) {
        Crc32::add(*address);
        address++;
    }

    // if flash is totally unprogrammed, this is a legitimate computed result,
    // but obviously does not represent a valid state we want to jump to
    if (Crc32::get() == 0xffffffff)
        return false;

    return (crc == Crc32::get());
}

/*
 * Branch to application code.
 * Assumes that the validity of the application has been verified.
 */
void Bootloader::jumpToApplication()
{
    // Initialize user application's stack pointer & jump to app's reset vector
    asm volatile(
        "mov    r3,  %[appMSP]          \n\t"
        "msr    msp, r3                 \n\t"
        "mov    r3, %[appResetVector]   \n\t"
        "bx     r3"
        :
        : [appMSP] "r"(*reinterpret_cast<uint32_t*>(APPLICATION_ADDRESS)),
            [appResetVector] "r"(APPLICATION_ADDRESS + 4)
    );
}
