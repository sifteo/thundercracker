
#include "bootloader.h"
#include "stm32flash.h"
#include "crc.h"
#include "tasks.h"
#include "usbprotocol.h"
#include "usb/usbdevice.h"
#include "aes128.h"
#include "stm32flash.h"

#include "string.h"

Bootloader::Update Bootloader::update;
bool Bootloader::firstLoad;
const uint32_t Bootloader::SIZE = 0x2000;
const uint32_t Bootloader::APPLICATION_ADDRESS = Stm32Flash::START_ADDR + SIZE;

void Bootloader::init()
{
    firstLoad = true;

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
    if (firstLoad) {
        UsbDevice::init();
        Stm32Flash::unlock();

        firstLoad = false;
    }

    ensureUpdaterIsEnabled();

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
        uint8_t plaintext[AES128::BLOCK_SIZE];
        const uint8_t *cipherIn = buf + 1;
        numBytes--; // step past the byte of command

        Stm32Flash::beginProgramming();

        while (numBytes >= AES128::BLOCK_SIZE) {
#if 0
            AES128::encryptBlock(plaintext, update.cipherBuf, update.expandedKey);
            AES128::xorBlock(plaintext, cipherIn);
            const uint16_t *p = reinterpret_cast<const uint16_t*>(plaintext);
#else
            // XXX: temp, handling unencrypted data only
            const uint16_t *p = reinterpret_cast<const uint16_t*>(cipherIn);
#endif
            const uint16_t *end = reinterpret_cast<const uint16_t*>(cipherIn + AES128::BLOCK_SIZE);

            while (p < end) {
                Stm32Flash::programHalfWord(*p, update.addressPointer);
                update.addressPointer += 2;
                p++;
            }

            cipherIn += AES128::BLOCK_SIZE;
            numBytes -= AES128::BLOCK_SIZE;
        }

        Stm32Flash::endProgramming();
        break;
    }

    case CmdSetAddrPtr: {

        if (numBytes < 5)
            break;

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

    case CmdWriteDetails: {

        if (numBytes < 9)
            break;

        uint32_t crc = *reinterpret_cast<const uint32_t*>(&buf[1]);
        uint32_t givenImgSize = *reinterpret_cast<const uint32_t*>(&buf[5]);
        uint32_t calculatedImgsize = update.addressPointer - APPLICATION_ADDRESS;

        if (givenImgSize != calculatedImgsize)
            break;

        Stm32Flash::beginProgramming();

        // write the CRC to the current address pointer
        Stm32Flash::programHalfWord(crc & 0xffff, update.addressPointer);
        Stm32Flash::programHalfWord((crc >> 16) & 0xffff, update.addressPointer + 2);

        // write the size to the end of MCU flash
        Stm32Flash::programHalfWord(givenImgSize & 0xffff, Stm32Flash::END_ADDR - 4);
        Stm32Flash::programHalfWord((givenImgSize >> 16) & 0xffff, Stm32Flash::END_ADDR - 2);

        Stm32Flash::endProgramming();
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

    while (a < Stm32Flash::END_ADDR) {
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
    Stm32Flash::eraseOptionBytes();
    Stm32Flash::setOptionByte(Stm32Flash::OptionDATA0, 0);
}

/*
 * If we're loading as a result of a failed CRC check, we must ensure that
 * the persistent updater bit is set.
 */
void Bootloader::ensureUpdaterIsEnabled()
{
    if (updaterIsEnabled())
        return;

    Stm32Flash::eraseOptionBytes();
    Stm32Flash::setOptionByte(Stm32Flash::OptionDATA0, 1);
}

/*
 * Validates the CRC of the contents of MCU flash.
 *
 * The bootloader must write the length of an installed image at the end of
 * MCU flash.
 */
bool Bootloader::mcuFlashIsValid()
{
    const uint32_t imgsize = *reinterpret_cast<uint32_t*>(Stm32Flash::END_ADDR - 4);
    if (imgsize > MAX_APP_SIZE)
        return false;

    Crc32::reset();
    const uint32_t *address = reinterpret_cast<uint32_t*>(APPLICATION_ADDRESS);
    const uint32_t *end = reinterpret_cast<uint32_t*>(APPLICATION_ADDRESS + imgsize);

    while (address < end) {
        Crc32::add(*address);
        address++;
    }

    // if flash is totally unprogrammed, this is a legitimate computed result,
    // but obviously does not represent a valid state we want to jump to
    const uint32_t calculatedCrc = Crc32::get();
    if (calculatedCrc == 0xffffffff || calculatedCrc == 0)
        return false;

    // CRC is stored in flash directly after the FW image
    const uint32_t storedCrc = *address;
    return (storedCrc == calculatedCrc);
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
