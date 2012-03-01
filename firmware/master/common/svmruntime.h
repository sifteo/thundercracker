/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

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
    SvmRuntime();  // Do not implement
    
    static void run(uint16_t appId);
    static void exit();
    static void call(reg_t addr);
    static void svc(uint8_t imm8);

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

    static bool loadElfFile(unsigned addr, unsigned len);

    static unsigned currentAppPhysAddr;    // where does this app start in external flash?
    static unsigned currentBlockValidBytes;
    static ProgramInfo progInfo;

    static void svcIndirectOperation(uint8_t imm8);
    static void addrOp(uint8_t opnum, reg_t address);
};

#endif // SVM_RUNTIME_H
