
#include "bootloader.h"
#include "stm32flash.h"
#include "crc.h"
#include "tasks.h"
#include "usbprotocol.h"
#include "usb/usbdevice.h"
#include "aes128.h"
#include "stm32flash.h"
#include "powermanager.h"
#include "systime.h"

#include "string.h"

Bootloader::Update Bootloader::update;
bool Bootloader::firstLoad;
const uint32_t Bootloader::APPLICATION_ADDRESS = Stm32Flash::START_ADDR + SIZE;

void Bootloader::init()
{
    firstLoad = true;
    Crc32::init();
    SysTime::init();
}

/*
 * High level program flow of the bootloader.
 */
void Bootloader::exec(bool userRequestedUpdate)
{
    if (userRequestedUpdate || manualUpdateRequested())
        load();

    while (!mcuFlashIsValid())
        load();

    // if we attempted to load, cleanup
    if (!firstLoad)
        cleanup();

    uint32_t msp = *reinterpret_cast<uint32_t*>(APPLICATION_ADDRESS);
    uint32_t resetVector = *reinterpret_cast<uint32_t*>(APPLICATION_ADDRESS + 4);
    jumpToApplication(msp, resetVector);
    // never comes back
}


/*
 * Provide a manual override of the standard startup process to account for the
 * scenario in which faulty firmware has been (successfully) loaded to a device.
 *
 * USB power must be applied, and home button must be pressed for 1 second.
 */
bool Bootloader::manualUpdateRequested()
{
    if (PowerManager::state() != PowerManager::UsbPwr)
        return false;

    GPIOPin homeButton = BTN_HOME_GPIO;
    homeButton.setControl(GPIOPin::IN_FLOAT);

    while (SysTime::ticks() < SysTime::sTicks(1)) {
        // active high - bail if released
        if (homeButton.isLow())
            return false;
    }

    return true;
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
        Tasks::init();
        UsbDevice::init();
        Stm32Flash::unlock();

        // Indicate we're loading
        GPIOPin red = LED_RED_GPIO;
        red.setControl(GPIOPin::OUT_2MHZ);
        red.setLow();

        firstLoad = false;
    }

    if (!eraseMcuFlash())
        return;

    // initialize decryption
    // XXX: select actual values
    const uint32_t key[4] = { 0x2b7e1516, 0x28aed2a6, 0xabf71588, 0x09cf4f3c };
    const uint8_t iv[] =    { 0x00, 0x01, 0x02, 0x03,   \
                              0x04, 0x05, 0x06, 0x07,   \
                              0x08, 0x09, 0x0a, 0x0b,   \
                              0x0c, 0x0d, 0x0e, 0x0f };
    AES128::expandKey(update.expandedKey, key);
    memcpy(update.cipherBuf, iv, AES128::BLOCK_SIZE);

    // initialize update state, default to beginning of application flash
    update.loadInProgress = true;
    update.addressPointer = APPLICATION_ADDRESS;

    while (update.loadInProgress)
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

    /*
     * Buf format: uint8_t command, [as many 16 byte aes blocks as fit]
     */
    case CmdWriteMemory: {
        uint8_t plaintext[AES128::BLOCK_SIZE];
        const uint8_t *cipherIn = buf + 1;
        numBytes--; // step past the byte of command

        Stm32Flash::beginProgramming();

        while (numBytes >= AES128::BLOCK_SIZE) {

            decryptBlock(plaintext, cipherIn);
            program(plaintext, AES128::BLOCK_SIZE);

            cipherIn += AES128::BLOCK_SIZE;
            numBytes -= AES128::BLOCK_SIZE;
        }

        Stm32Flash::endProgramming();
        break;
    }

    /*
     * Buf format: uint8_t command, 16 bytes final aes block, uint32_t CRC, uint32_t size
     */
    case CmdWriteFinal: {

        if (numBytes < 25)
            break;

        const uint8_t *cipherIn = buf + 1;
        uint8_t plaintext[AES128::BLOCK_SIZE];
        decryptBlock(plaintext, cipherIn);

        Stm32Flash::beginProgramming();

        // last byte of last block is always the pad value
        uint8_t padvalue = plaintext[AES128::BLOCK_SIZE - 1];
        program(plaintext, AES128::BLOCK_SIZE - padvalue);

        /*
         * Write details to allow us to verify contents of MCU flash.
         */
        uint32_t crc = *reinterpret_cast<const uint32_t*>(buf + 17);
        uint32_t size = *reinterpret_cast<const uint32_t*>(buf + 21);

        Stm32Flash::programHalfWord(crc & 0xffff, update.addressPointer);
        Stm32Flash::programHalfWord((crc >> 16) & 0xffff, update.addressPointer + 2);

        Stm32Flash::programHalfWord(size & 0xffff, Stm32Flash::END_ADDR - 4);
        Stm32Flash::programHalfWord((size >> 16) & 0xffff, Stm32Flash::END_ADDR - 2);

        Stm32Flash::endProgramming();
        update.loadInProgress = false;
        break;
    }

    case CmdResetAddrPtr:
        update.addressPointer = APPLICATION_ADDRESS;
        break;

    case CmdGetAddrPtr: {
        uint8_t response[5] = { buf[0] };
        memcpy(response + 1, &update.addressPointer, sizeof update.addressPointer);
        UsbDevice::write(response, sizeof response);
        break;
    }

    }
}

/*
 * Decrypt a single block of AES encrypted data.
 * AES128::BLOCK_SIZE bytes must be available.
 */
void Bootloader::decryptBlock(uint8_t *plaintext, const uint8_t *cipher)
{
    AES128::encryptBlock(plaintext, update.cipherBuf, update.expandedKey);
    memcpy(update.cipherBuf, cipher, AES128::BLOCK_SIZE);
    AES128::xorBlock(plaintext, update.cipherBuf);
}

/*
 * Program the requested data to the current address pointer.
 */
void Bootloader::program(const uint8_t *data, unsigned len)
{
    const uint16_t *p = reinterpret_cast<const uint16_t*>(data);
    const uint16_t *end = reinterpret_cast<const uint16_t*>(data + len);
    while (p < end) {
        Stm32Flash::programHalfWord(*p, update.addressPointer);
        update.addressPointer += 2;
        p++;
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
    const uint32_t *end = address + (imgsize / sizeof(uint32_t));

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
 * Any deinit or cleanup required before we jump to application code.
 */
void Bootloader::cleanup()
{
    // ensure LED is off
    GPIOPin red = LED_RED_GPIO;
    red.setHigh();
}

/*
 * Branch to application code.
 * Assumes that the validity of the application has been verified.
 */
void Bootloader::jumpToApplication(uint32_t msp, uint32_t resetVector)
{
    // set the MSP and jump!
    asm volatile(
        "msr    msp, r0     \n\t"
        "bx     r1"
    );
}
