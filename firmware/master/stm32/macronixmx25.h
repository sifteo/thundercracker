/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef MACRONIX_MX25_H
#define MACRONIX_MX25_H

#include "spi.h"
#include "flash_device.h"

class MacronixMX25
{
public:
    enum Status {
        Ok                  = 0,
        // from status register
        WriteInProgress     = (1 << 0),
        WriteEnableLatch    = (1 << 1),
        // from security register
        ProgramFail         = (1 << 5),
        EraseFail           = (1 << 6),
        WriteProtected      = (1 << 7)
    };

    MacronixMX25(GPIOPin _csn, SPIMaster _spi) :
        csn(_csn), spi(_spi)
    {}

    void init();

    void read(uint32_t address, uint8_t *buf, unsigned len);
    void write(uint32_t address, const uint8_t *buf, unsigned len);
    void eraseBlock(uint32_t address);
    void chipErase();

    bool busy() {
        if (mightBeBusy) {
            mightBeBusy = readReg(ReadStatusReg) & WriteInProgress;
        }
        return mightBeBusy || dmaInProgress;
    }

    void readId(FlashDevice::JedecID *id);

    void deepSleep();
    void wakeFromDeepSleep();

    // should only be called from flash_device.cpp
    static void dmaCompletionCallback();

private:
    enum Command {
        Read                        = 0x03,
        FastRead                    = 0x0B,
        Read2                       = 0xBB,
        Read4                       = 0xEB,
        ReadW4                      = 0xE7,
        PageProgram                 = 0x02,
        QuadPageProgram             = 0x38,
        SectorErase                 = 0x20,
        BlockErase32                = 0x52,
        BlockErase64                = 0xD8,
        ChipErase                   = 0xC7,
        WriteEnable                 = 0x06,
        WriteDisable                = 0x04,
        ReadStatusReg               = 0x05,
        ReadConfigReg               = 0x15,
        WriteStatusConfigReg        = 0x01,
        WriteProtectSelect          = 0x68,
        EnableQpi                   = 0x35,
        ResetQpi                    = 0xF5,
        SuspendProgramErase         = 0x30,
        DeepPowerDown               = 0xB9,
        ReleaseDeepPowerDown        = 0xAB,  // Also includes ReadElectronicID
        SetBurstLength              = 0xC0,
        ReadID                      = 0x9F,
        ReadMfrDeviceID             = 0x90,
        ReadUntilCSNHigh            = 0x5A,
        EnterSecureOtp              = 0xB1,
        ExitSecureOtp               = 0xC1,
        ReadSecurityReg             = 0x2B,
        WriteSecurityReg            = 0x2F,
        SingleBlockLock             = 0x36,
        SingleBlockUnlock           = 0x39,
        ReadBlockLock               = 0x3C,
        GangBlockLock               = 0x7E,
        GangBlockUnlock             = 0x98,
        Nop                         = 0x0,
        ResetEnable                 = 0x66,
        Reset                       = 0x99,
        ReleaseReadEnhanced         = 0xFF
    };

    GPIOPin csn;
    SPIMaster spi;
    bool mightBeBusy;

    ALWAYS_INLINE void spiBegin() {
        csn.setLow();
    }

    ALWAYS_INLINE void spiEnd() {
        csn.setHigh();
    }

    void ensureWriteEnabled();
    void waitWhileBusy();
    bool waitForDma();
    static volatile bool dmaInProgress;

    uint8_t readReg(Command cmd);
};

#endif // MACRONIX_MX25_H
