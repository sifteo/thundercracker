#include "sdiohost.h"
#include "dma.h"

void SdioHost::init(const Config &cfg)
{
    // only one SDIO peripheral on STM32F10x
    RCC.AHBENR |= (1 << 10);

    dmaTx = &DMA2.channels[3];  // DMA2, channel 4
    //    Dma::initChannel(&DMA2, 3, dmaCallback, this);

    // point DMA at our data fifo
    dmaTx->CPAR = reinterpret_cast<uint32_t>(&hw->FIFO);

    // Configuration, card clock is initially stopped
    hw->POWER  = 0;
    hw->CLKCR  = 0;
    hw->DCTRL  = 0;
    hw->DTIMER = 0;
}

void SdioHost::startClock()
{
    // Initial clock setting: 400kHz, 1bit mode.
    // SDIO_CK frequency = SDIOCLK / [CLKDIV + 2]
    hw->CLKCR  = LOW_SPEED_DIV; // clock is set to low speed
    hw->POWER  = 0x3;           // power on
    hw->CLKCR |= (1 << 8);      // CLKEN - clock is enabled
}

void SdioHost::setDataClock()
{
    /*
     * Must be 25MHz or less.
     */
    hw->CLKCR = (hw->CLKCR & 0xFFFFFF00) | HIGH_SPEED_DIV;
}

void SdioHost::stopClock()
{
    hw->CLKCR = 0;
    hw->POWER = 0;
}

void SdioHost::setBusMode(BusMode mode)
{
    hw->CLKCR = (hw->CLKCR & ~ModeMask) | mode;
}

/*
 * XXX: update these CMD routines to avoid spinning,
 *      and to avoid duplication if possible
 */

void SdioHost::sendCmdNone(uint8_t cmd, uint32_t arg)
{
    /*
     * Send a command that requires no response.
     */

    hw->ARG = arg;
    hw->CMD = (uint32_t)cmd | (1 << 10);    // CPSMEN: Command path state machine enable
    while ((hw->STA & (1 << 7)) == 0)       // CMDSENT: Command sent(no response required)
        ;
    hw->ICR = (1 << 7);                     // CMDSENTC: CMDSENT flag clear bit
}

bool SdioHost::sendCmdShort(uint8_t cmd, uint32_t arg, uint32_t *resp)
{
    /*
     * Send a command with a short response expected.
     */

    hw->ARG = arg;
    hw->CMD = (uint32_t)cmd |
            (1 << 7) |  // WAITRESP 01: Short response, expect CMDREND or CCRCFAIL
            (1 << 10);  // CPSMEN: Command path state machine enable

    uint32_t sta;
    uint32_t doneTimeoutOrFail = (1 << 6) | (1 << 2) | (1 << 0);
    while (((sta = hw->STA) & doneTimeoutOrFail) == 0)
        ;
    hw->ICR = (1 << 6) |    // CMDRENDC
              (1 << 2) |    // CTIMEOUTC
              (1 << 0);     // CCRCFAILC
    if (sta & (1 << 2)) {   // CTIMEOUT
        // TODO: capture the error bits here?
        hw->ICR = 0xffffffff;
        return false;
    }

    *resp = hw->RESP1;
    return true;
}

bool SdioHost::sendCmdShortCrc(uint8_t cmd, uint32_t arg, uint32_t *resp)
{
    hw->ARG = arg;
    hw->CMD = (uint32_t)cmd |
            (1 << 6) |  // WAITRESP 01: Short response, expect CMDREND or CCRCFAIL
            (1 << 10);  // CPSMEN: Command path state machine enable

    uint32_t sta;
    uint32_t doneTimeoutOrFail = (1 << 6) | (1 << 2) | (1 << 0);
    while (((sta = hw->STA) & doneTimeoutOrFail) == 0)
        ;
    hw->ICR = (1 << 6) |    // CMDRENDC
              (1 << 2) |    // CTIMEOUTC
              (1 << 0);     // CCRCFAILC

    if (sta & ((1 << 2) | (1 << 0))) {
        // TODO: capture the error bits here?
        hw->ICR = 0xffffffff;
        return false;
    }
    *resp = hw->RESP1;
    return true;
}

bool SdioHost::sendCmdLongCrc(uint8_t cmd, uint32_t arg, uint32_t *resp)
{
    hw->ARG = arg;
    hw->CMD = (uint32_t)cmd |
            (3 << 6) |  // WAITRESP 11: Long response, expect CMDREND or CCRCFAIL
            (1 << 10);  // CPSMEN: Command path state machine enable

    uint32_t sta;
    uint32_t doneTimeoutOrFail = (1 << 6) | (1 << 2) | (1 << 0);
    while (((sta = hw->STA) & doneTimeoutOrFail) == 0)
        ;
    hw->ICR = (1 << 6) |    // CMDRENDC
              (1 << 2) |    // CTIMEOUTC
              (1 << 0);     // CCRCFAILC

    uint32_t errMask =  (1 << 0) | // CCRCFAIL
                        (1 << 1) | // DCRCFAIL
                        (1 << 2) | // CTIMEOUT
                        (1 << 3) | // DTIMEOUT
                        (1 << 4) | // TXUNDERR
                        (1 << 5);  // RXOVERR
    if (sta & errMask) {
        // TODO: capture the error bits here?
        hw->ICR = 0xffffffff;
        return false;
    }

    // MSB in response comes first
    *resp++ = hw->RESP4;
    *resp++ = hw->RESP3;
    *resp++ = hw->RESP2;
    *resp   = hw->RESP1;
    return true;
}
