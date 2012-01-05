#ifndef MACRONIX_MX25_H
#define MACRONIX_MX25_H

#include "spi.h"

class MacronixMX25
{
public:
    static const unsigned PAGE_SIZE = 256;      // programming granularity
    static const unsigned SECTOR_SIZE = 4096;   // erase granularity

    static MacronixMX25 instance;

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

    MacronixMX25(SPIMaster _spi) :
        spi(_spi)
    {}

    void init();

    void read(uint32_t address, uint8_t *buf, unsigned len);
    Status write(uint32_t address, const uint8_t *buf, unsigned len);
    Status eraseSector(uint32_t address);

    void deepSleep();
    void wakeFromDeepSleep();

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
        ReleaseDeepPowerDown        = 0xAB,
        SetBurstLength              = 0xC0,
        ReadID                      = 0x9F,
//        ReadElectronicID            = 0xAB, // this included in ReleaseDeepPowerDown
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

    typedef struct JedecID_t {
        uint8_t manufacturerID;
        uint8_t memoryType;
        uint8_t memoryDensity;
    } JedecID;

    SPIMaster spi;

    void ensureWriteEnabled();
    uint8_t readReg(Command cmd);
};

#endif // MACRONIX_MX25_H
