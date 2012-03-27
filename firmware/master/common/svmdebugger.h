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
    
    // Map of *flash addresses* to set breakpoints on.
    uint32_t breakpointMap;
    uint32_t breakpoints[Svm::Debugger::NUM_BREAKPOINTS];

    void handleMessage(SvmDebugPipe::DebuggerMsg &msg);

    void msgReadRegisters(SvmDebugPipe::DebuggerMsg &msg);
    void msgWriteRegisters(SvmDebugPipe::DebuggerMsg &msg);
    void msgReadRAM(SvmDebugPipe::DebuggerMsg &msg);
    void msgWriteRAM(SvmDebugPipe::DebuggerMsg &msg);
    void msgSignal(SvmDebugPipe::DebuggerMsg &msg);
    void msgIsStopped(SvmDebugPipe::DebuggerMsg &msg);
    void msgDetach(SvmDebugPipe::DebuggerMsg &msg);
    void msgSetBreakpoints(SvmDebugPipe::DebuggerMsg &msg);

    void setUserReg(uint32_t r, uint32_t value);
    uint32_t getUserReg(uint32_t r);

    static void breakpointPatch(uint8_t *data, uint32_t offset);
};

#endif // SVM_DEBUGGER_H
