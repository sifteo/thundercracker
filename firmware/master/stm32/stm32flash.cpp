#include "stm32flash.h"

/*
 * Clear out all the option bytes.
 * Specify RDP while we're at it, since its default state (erased) results
 * in it being enabled.
 */
bool Stm32Flash::eraseOptionBytes(bool enableRDP)
{
    if (waitForPreviousOperation() != WaitOk)
        return false;

    unlockOptionBytes();

    // erase option bytes
    FLASH.CR |= (1 << 5);   // OPTER: Option byte erase
    FLASH.CR |= (1 << 6);   // STRT: start the erase operation
    bool success = (waitForPreviousOperation() == WaitOk);
    FLASH.CR &= ~(1 << 5);  // OPTER: Option byte erase
    if (!success)
        return false;

    // keep readout protect disabled so we can program option bytes
    FLASH.CR |= (1 << 4);
    FLASH_OB.RDP = enableRDP ? 0 : RDP_DISABLE_KEY;
    success = (waitForPreviousOperation() == WaitOk);
    FLASH.CR &= ~(1 << 4);

    return success;
}

/*
 *
 */
bool Stm32Flash::setOptionByte(OptionByte ob, uint16_t value)
{
    if (waitForPreviousOperation() != WaitOk)
        return false;

    unlockOptionBytes();

    // program option bytes
    FLASH.CR |= (1 << 4);   // OPTPG: Option byte programming
    uint32_t optionByteAddr = reinterpret_cast<uint32_t>(&FLASH_OB);
    optionByteAddr += (ob * sizeof(uint16_t));

    beginProgramming();
    programHalfWord(value, optionByteAddr);
    bool success = (waitForPreviousOperation() == WaitOk);
    FLASH.CR &= ~(1 << 4);  // OPTPG: Option byte programming
    endProgramming();

    return success;
}

/*
 * Flash hardware programs one half word at a time.
 * beginProgramming() must have been called prior.
 */
bool Stm32Flash::programHalfWord(uint16_t halfword, uint32_t address)
{
    if (waitForPreviousOperation() != WaitOk)
        return false;

    *reinterpret_cast<volatile uint16_t*>(address) = halfword;

    return true;
}

Stm32Flash::WaitStatus Stm32Flash::waitForPreviousOperation()
{
    // wait for busy, but break on error
    while ((FLASH.SR & 0x1f) == 0x1)
        ;

    return (FLASH.SR & 0x1f) ? WaitError : WaitOk;
}

bool Stm32Flash::erasePage(uint32_t address)
{
    if (waitForPreviousOperation() != WaitOk)
        return false;

    FLASH.CR |= (1 << 1);   // PER: page erase
    FLASH.AR = address;
    FLASH.CR |= (1 << 6);   // STRT: start the erase operation
    bool success = (waitForPreviousOperation() == WaitOk);
    FLASH.CR &= ~(1 << 1);  // PER: disable page erase

    return success;
}
