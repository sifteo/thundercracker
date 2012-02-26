#ifndef SVM_RUNTIME_H
#define SVM_RUNTIME_H

#include <stdint.h>
#include <inttypes.h>

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
    void call(reg_t addr);
    void svc(uint8_t imm8);

    // address translation routines
    reg_t virt2physRam(uint32_t vaddr) const;
    reg_t virt2cacheFlash(uint32_t a) const;
    reg_t cache2virtFlash(reg_t a) const;
    reg_t cacheBlockBase() const;

    inline bool inRange(reg_t val, reg_t start, reg_t sz) const {
        return (val - start < sz) ? true : false;
    }

    // TODO: evaluate whether we can standardize on a pointer type for validate
    // input and get rid of templates
    /*
        Ensure that a read-only pointer is valid, and translate if necessary.
        Read-write pointers may reference any valid flash or ram memory.
    */
    template <typename T>
    bool validateReadOnly(T &ptr, uint32_t size, bool allowNULL = false)
    {
        if (!allowNULL && !ptr)
            return false;

        reg_t addr = reinterpret_cast<reg_t>(ptr);
        if (isFlashAddr(addr)) {
            addr = virt2cacheFlash(addr);
            if (!inRange(addr, cacheBlockBase(), FlashLayer::BLOCK_SIZE)) {
                FlashLayer::releaseRegion(flashRegion);
                fetchFlashBlock(reinterpret_cast<reg_t>(ptr));
            }
            // set base pointers
            cpu.setReg(8, addr);
            cpu.setReg(9, 0);   // r9 is read/write, which is invalid for flash pointers
        }
        else {
            // if it's already in range, leave it be
            if (!inRange(addr, cpu.userRam(), SvmCpu::MEM_IN_BYTES))
                addr = virt2physRam(addr);
            ASSERT(inRange(addr, cpu.userRam(), SvmCpu::MEM_IN_BYTES) && "validateReadOnly: bad address");
            // set base pointers
            cpu.setReg(8, addr);
            cpu.setReg(9, addr);
        }

        LOG(("validated: 0x%"PRIxPTR" for address 0x%"PRIxPTR"\n", addr, reinterpret_cast<reg_t>(ptr)));
        ptr = reinterpret_cast<T>(addr);
        return true;
    }

    /*
        Ensure that a read-write pointer is valid, and translate if necessary.
        Read-write pointers cannot reference any flash memory.
    */
    template <typename T>
    bool validateReadWrite(T &ptr, uint32_t size, bool allowNULL = false)
    {
        if (!allowNULL && !ptr)
            return false;

        reg_t addr = reinterpret_cast<reg_t>(ptr);
        if (isFlashAddr(addr))
            return false;

        // if it's already in range, leave it be
        if (!inRange(addr, cpu.userRam(), SvmCpu::MEM_IN_BYTES))
            addr = virt2physRam(addr);
        ASSERT(inRange(addr, cpu.userRam(), SvmCpu::MEM_IN_BYTES) && "validateReadWrite: bad address");

        // set base pointers
        cpu.setReg(8, addr);
        cpu.setReg(9, addr);
        ptr = reinterpret_cast<T>(addr);
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

    struct StackFrame {
        reg_t r7;
        reg_t r6;
        reg_t r5;
        reg_t r4;
        reg_t r3;
        reg_t r2;
        reg_t fp;
        reg_t sp;
    };

    bool loadElfFile(unsigned addr, unsigned len);

    inline bool isFlashAddr(reg_t a) const {
        return (a & (1 << 31));
    }

    void fetchFlashBlock(reg_t addr);

    SvmCpu cpu;

    FlashRegion flashRegion;
    unsigned currentAppPhysAddr;    // where does this app start in external flash?
    unsigned currentBlockValidBytes;
    ProgramInfo progInfo;

    reg_t validate(reg_t address);
    void svcIndirectOperation(uint8_t imm8);
    void addrOp(uint8_t opnum, reg_t address);
};

#endif // SVM_RUNTIME_H
