
#include "bootloader.h"
#include "crc.h"

Bootloader::Bootloader()
{
    
}

void Bootloader::init()
{
    
}

/*
 * High level program flow of the bootloader.
 */
void Bootloader::exec()
{
    if (updaterIsEnabled()) {
        load();
    }

    while (!mcuFlashIsValid())
        load();

    jumpToApplication();
}

bool Bootloader::load()
{
    return false;
}

/*
 * Returns whether the updater functionality is enabled or not.
 * This is a persistent setting that can only be reset once we have
 * successfully installed a new update.
 */
bool Bootloader::updaterIsEnabled()
{
    return false;
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
