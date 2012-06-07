#include "stm32flash.h"

/*
 * optionByteTable provides a table of { bool, value } entries
 * to program into the option bytes.
 */
bool Stm32Flash::setOptionBytes(OptionByte *optionBytes)
{
    if (waitForPreviousOperation() != WaitOk)
        return false;

    // enable information block programming
    FLASH.OPTKEYR = KEY1;
    FLASH.OPTKEYR = KEY2;

    // erase option bytes
    FLASH.CR |= (1 << 5);   // OPTER: Option byte erase
    FLASH.CR |= (1 << 6);   // STRT: start the erase operation
    bool success = (waitForPreviousOperation() == WaitOk);
    FLASH.CR &= ~(1 << 5);  // OPTER: Option byte erase
    if (!success)
        return false;

    // program option bytes
    FLASH.CR |= (1 << 4);   // OPTPG: Option byte programming

    uint32_t OB = reinterpret_cast<uint32_t>(&FLASH_OB);
    for (unsigned i = 0; i < NUM_OPTION_BYTES; ++i) {
        if (optionBytes->program)
            programHalfWord(optionBytes->value, OB);
        optionBytes++;
        OB += sizeof(uint16_t);
    }

    waitForPreviousOperation();
    FLASH.CR &= ~(1 << 4);  // OPTPG: Option byte programming

    return true;
}

/*
 * Flash hardware programs one half word at a time.
 * beginProgramming() must have been called prior.
 */
bool Stm32Flash::programHalfWord(uint16_t halfword, uint32_t address)
{
    if (waitForPreviousOperation() != WaitOk)
        return false;

    volatile uint16_t *pData = reinterpret_cast<uint16_t*>(address);
    *pData = halfword;

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

    FLASH.AR = address;
    FLASH.CR |= (1 << 6);   // STRT: start the erase operation

    return true;
}
