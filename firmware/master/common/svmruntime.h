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

    // Hypercall entry point, called by low-level SvmCpu code.
    static void svc(uint8_t imm8);

    /**
     * Call Event::Dispatch() on our way out of the next svc(). Events can't
     * be dispatched while we're in syscalls, since the internal call() we
     * generate must occur *after* the syscall's return value has been stored.
     */
    static void dispatchEventsOnReturn() {
        eventDispatchFlag = true;
    }

    /**
     * Using the current codeBlock and PC, reconstruct the current
     * virtual program counter. Used for debugging, and for function calls.
     */
    static unsigned reconstructCodeAddr() {
        return SvmMemory::reconstructCodeAddr(codeBlock, SvmCpu::reg(SvmCpu::REG_PC));
    }
    
    /**
     * Event dispatch is only possible when the PC is at a bundle boundary,
     * since the resulting return pointer would not be valid coming from
     * untrusted stack memory, and when it's not already in an event handler.
     */
    static bool canSendEvent() {
        return eventFrame == 0 && (SvmCpu::reg(SvmCpu::REG_PC) & 3) == 0;
    }
    
    /**
     * Event dispatch is just a call() which also slips new parameters
     * into the registers after saving our CallFrame. This does not affect
     * CPU state at the call site.
     *
     * Only one event may be dispatched at a time; this call is actually
     * just setting up the VM to run an event handler next time we return to
     * user code. We choose not to stack event handlers; instead, to use
     * less memory and behave more predictably, we run one handler at a time
     * and, on return, we evaluate whether there need to be any further event
     * dispatches before returning to the main user thread.
     */
    static void sendEvent(reg_t addr) {
        ASSERT(canSendEvent());
        call(addr);
        eventFrame = SvmCpu::reg(SvmCpu::REG_FP);
    }
    static void sendEvent(reg_t addr, reg_t r0) {
        sendEvent(addr);
        SvmCpu::setReg(0, r0);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1) {
        sendEvent(addr, r0);
        SvmCpu::setReg(1, r1);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1, reg_t r2) {
        sendEvent(addr, r0, r1);
        SvmCpu::setReg(2, r2);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1, reg_t r2, reg_t r3) {
        sendEvent(addr, r0, r1, r2);
        SvmCpu::setReg(3, r3);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1, reg_t r2, reg_t r3, reg_t r4) {
        sendEvent(addr, r0, r1, r2, r3);
        SvmCpu::setReg(4, r4);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1, reg_t r2, reg_t r3, reg_t r4, reg_t r5) {
        sendEvent(addr, r0, r1, r2, r3, r4);
        SvmCpu::setReg(5, r5);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1, reg_t r2, reg_t r3, reg_t r4, reg_t r5, reg_t r6) {
        sendEvent(addr, r0, r1, r2, r3, r4, r5);
        SvmCpu::setReg(6, r6);
    }
    static void sendEvent(reg_t addr, reg_t r0, reg_t r1, reg_t r2, reg_t r3, reg_t r4, reg_t r5, reg_t r6, reg_t r7) {
        sendEvent(addr, r0, r1, r2, r3, r4, r5, r6);
        SvmCpu::setReg(7, r7);
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
    static reg_t eventFrame;
    static bool eventDispatchFlag;

    static void call(reg_t addr);
    static void tailcall(reg_t addr);    
    static void ret();

    static void resetSP();
    static void adjustSP(int words);
    static void setSP(reg_t addr);
    
    static void longLDRSP(unsigned reg, unsigned offset);
    static void longSTRSP(unsigned reg, unsigned offset);

    static void enterFunction(reg_t addr);
    static void branch(reg_t addr);
    static void validate(reg_t addr);
    static void syscall(unsigned num);
    static void svcIndirectOperation(uint8_t imm8);
    static void addrOp(uint8_t opnum, reg_t addr);
};

#endif // SVM_RUNTIME_H
