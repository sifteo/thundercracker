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
#include "svmmemory.h"
#include "flashlayer.h"

using namespace Svm;

class SvmRuntime {
public:
    SvmRuntime();  // Do not implement

    static void run(uint16_t appId);
    static void exit();

    static void svc(uint8_t imm8);
    static void call(reg_t addr);
    static void tailcall(reg_t addr);    
    static void ret();
    
    /**
     * Using the current codeBlock and PC, reconstruct the current
     * virtual program counter. Used for debugging, and for function calls.
     */
    static unsigned reconstructCodeAddr() {
        return SvmMemory::reconstructCodeAddr(codeBlock, SvmCpu::reg(SvmCpu::REG_PC));
    }

private:
    struct CallFrame {
        uint32_t pc;
        uint32_t fp;
        uint32_t r2;
        uint32_t r3;
        uint32_t r4;
        uint32_t r5;
        uint32_t r6;
        uint32_t r7;
    };

    static FlashBlockRef codeBlock;
    static FlashBlockRef dataBlock;
    static SvmMemory::PhysAddr stackLimit;

    static void resetSP();
    static void adjustSP(int words);
    static void setSP(reg_t addr);

    static void enterFunction(reg_t addr);
    static void branch(reg_t addr);
    static void validate(reg_t addr);
    static void syscall(unsigned num);
    static void svcIndirectOperation(uint8_t imm8);
    static void addrOp(uint8_t opnum, reg_t addr);
};

#endif // SVM_RUNTIME_H
