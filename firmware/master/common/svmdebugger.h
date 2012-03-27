/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Debug stub for the SVM runtime.
 */

#ifndef SVM_DEBUGGER_H
#define SVM_DEBUGGER_H

#include <stdint.h>
#include <inttypes.h>
#include "svm.h"
#include "svmdebugpipe.h"
#include "svmmemory.h"


class SvmDebugger {
public:
    static void messageLoop(void *param=0);

    /// Returns true if we stopped the target with the indicated signal,
    /// false if the debugger is not attached.
    static bool signal(Svm::Debugger::Signals sig);

    /// Handle an SVM fault. Returns 'true' if the fault can be handled.
    static bool fault(Svm::FaultCode code);

    /// When paging in a flash block, we may need to patch it to apply breakpoints.
    static void patchFlashBlock(uint32_t blockAddr, uint8_t *data);

private:
    SvmDebugger() {}
    static SvmDebugger instance;

    Svm::Debugger::Signals stopped;
    bool attached;
    bool inMessageLoop;
    
    // Map of *flash addresses* to set breakpoints on.
    // We have one non-client-visible breakpoint, for implementing
    // single-step through normal native instructions.
    
    static const uint32_t NUM_USER_BREAKPOINTS = Svm::Debugger::NUM_BREAKPOINTS;
    static const uint32_t NUM_TOTAL_BREAKPOINTS = NUM_USER_BREAKPOINTS + 1;
    static const uint32_t STEP_BREAKPOINT = NUM_USER_BREAKPOINTS;
    uint32_t breakpointMap;
    uint32_t breakpoints[NUM_TOTAL_BREAKPOINTS];

    void messageLoopWork();
    void handleMessage(SvmDebugPipe::DebuggerMsg &msg);

    void msgReadRegisters(SvmDebugPipe::DebuggerMsg &msg);
    void msgWriteRegisters(SvmDebugPipe::DebuggerMsg &msg);
    void msgReadRAM(SvmDebugPipe::DebuggerMsg &msg);
    void msgWriteRAM(SvmDebugPipe::DebuggerMsg &msg);
    void msgSignal(SvmDebugPipe::DebuggerMsg &msg);
    void msgIsStopped(SvmDebugPipe::DebuggerMsg &msg);
    void msgDetach(SvmDebugPipe::DebuggerMsg &msg);
    void msgSetBreakpoints(SvmDebugPipe::DebuggerMsg &msg);
    void msgStep(SvmDebugPipe::DebuggerMsg &msg);

    void setUserReg(uint32_t r, uint32_t value);
    uint32_t getUserReg(uint32_t r);

    void breakpointsChanged();
    static void breakpointPatch(uint8_t *data, uint32_t offset);
    void setStepBreakpoint(SvmMemory::VirtAddr va);
};

#endif // SVM_DEBUGGER_H
