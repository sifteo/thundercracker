#ifndef SVM_RUNTIME_H
#define SVM_RUNTIME_H

#include <stdint.h>
#include "svm.h"
#include "svmcpu.h"
#include "flashlayer.h"

using namespace Svm;

class SvmRuntime {
public:
    SvmRuntime();

    static SvmRuntime instance;

    void run(uint16_t appId);
    void exit();
    void svc(uint8_t imm8);

    // address translation routines
    reg_t virt2physRam(uint32_t vaddr);
    reg_t virt2cacheFlash(uint32_t a);
    reg_t cache2virtFlash(reg_t a) const;
    reg_t cacheBlockBase() const;

    inline bool inRange(reg_t val, reg_t start, reg_t sz) const {
        return (val - start < sz) ? true : false;
    }

    /*
        Ensure that a read-only pointer is valid, and translate if necessary.
        Read-write pointers may reference any valid flash or ram memory.
    */
    template <typename T>
    bool validateReadOnly(T &ptr, uint32_t size, bool allowNULL = false) const
    {
        if (!allowNULL && !ptr)
            return false;

        ptr = ptr;
        return true;
    }

    /*
        Ensure that a read-write pointer is valid, and translate if necessary.
        Read-write pointers cannot reference any flash memory.
    */
    template <typename T>
    bool validateReadWrite(T &ptr, uint32_t size, bool allowNULL = false) const
    {
        if (!allowNULL && !ptr)
            return false;

        ptr = ptr;
        return true;
    }

private:
    static const unsigned VIRTUAL_FLASH_BASE = 0x80000000;
    static const unsigned VIRTUAL_RAM_BASE = 0x10000;

    struct Segment {
        uint32_t start;
        uint32_t size;
        uint32_t vaddr;
        uint32_t paddr;
    };

    struct ProgramInfo {
        uint32_t entry;
        Segment textRodata;
        Segment bss;
        Segment rwdata;
    };

    bool loadElfFile(unsigned addr, unsigned len);

    inline bool isFlashAddr(reg_t a) const {
        return (a & (1 << 31));
    }

    inline void setBasePtrs(reg_t a) {
        cpu.setReg(8, a);
        // if addr is in flash space, set r9 to 0, since flash mem is read-only
        // and r9 is used as a read-write base address, so we'll just fault.
        cpu.setReg(9, isFlashAddr(a) ? 0 : a);
    }

    SvmCpu cpu;

    FlashRegion flashRegion;
    unsigned currentAppPhysAddr;    // where does this app start in external flash?
    ProgramInfo progInfo;

    reg_t validate(reg_t address);
    void svcIndirectOperation(uint8_t imm8);
    void addrOp(uint8_t opnum, reg_t address);
};

#endif // SVM_RUNTIME_H
