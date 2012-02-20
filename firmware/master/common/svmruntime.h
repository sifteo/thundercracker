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

    void run(uint16_t appId);
    void svc(uint8_t imm8);

private:
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

    SvmCpu cpu;

    FlashRegion flashRegion;
    unsigned currentAppPhysAddr;    // where does this app start in external flash?
    ProgramInfo progInfo;

    reg_t validate(uint32_t address);
    void svcIndirectOperation(uint8_t imm8);
    void addrOp(uint8_t opnum, reg_t address);
};

#endif // SVM_RUNTIME_H
