/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _STM32_I2C_H
#define _STM32_I2C_H

#include <stdint.h>
#include "hardware.h"
#include "gpio.h"

class I2CSlave {
public:
    I2CSlave (volatile I2C_t *_hw) :
        hw(_hw) {}

    /*
     * SR1 is in the low 16 bits, SR2 in the high 16 bits
     */
    enum StatusFlags {

        // SR2 - high 16 bits
        TxRx            = (1 << 2) << 16,
        Busy            = (1 << 1) << 16,

        // SR1 - low 16 bits
        PECErr          = (1 << 12),
        OverUnderRun    = (1 << 11),
        Nack            = (1 << 10),
        TxEmpty         = (1 << 7),
        RxNotEmpty      = (1 << 6),
        StopBit         = (1 << 4),
        ByteTxFinished  = (1 << 2),
        AddressMatch    = (1 << 1)
    };

    void init(GPIOPin scl, GPIOPin sda, uint8_t address);

    // helper combos
    static const uint32_t RxAddressMatched = Busy | AddressMatch;
    static const uint32_t TxAddressMatched = TxRx | Busy | TxEmpty | AddressMatch;

    static const uint32_t ByteReceived = Busy | RxNotEmpty;
    static const uint32_t ByteTransmitted = TxRx | Busy | TxEmpty | ByteTxFinished;

    ALWAYS_INLINE uint32_t status() const {
        /*
         * On an AddressMatch event, ADDR is cleared when SR1 is read, followed
         * by a read of SR2. Might as well do that each time we access status.
         *
         * The returned mask is compatible with StatusFlags
         */
        uint16_t sr1 = hw->SR1;
        uint16_t sr2 = hw->SR2;
        return ((sr2 << 16) | sr1) & 0x00FFFFFF;
    }

    void isrER();

    ALWAYS_INLINE uint8_t read() {
        return hw->DR;
    }

    ALWAYS_INLINE void write(uint8_t byte) {
        hw->DR = byte;
    }

    ALWAYS_INLINE void onStopBit() {
        // second step to clear STOP bit is to write to CR1.
        // just write something harmless
        hw->CR1 |= 0x1;
    }

private:
    volatile I2C_t *hw;
};

#endif
