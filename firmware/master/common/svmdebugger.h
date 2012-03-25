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
    static void handleBreakpoint(void *param=0);

private:
    SvmDebugger() {}
    static SvmDebugger instance;

    bool stopped;

    void handleMessage(SvmDebugPipe::DebuggerMsg &msg);

    void readRegisters(SvmDebugPipe::DebuggerMsg &msg);
    void writeRegisters(SvmDebugPipe::DebuggerMsg &msg);
    void writeSingleReg(SvmDebugPipe::DebuggerMsg &msg);
    void readRAM(SvmDebugPipe::DebuggerMsg &msg);
    void writeRAM(SvmDebugPipe::DebuggerMsg &msg);
};

#endif // SVM_DEBUGGER_H
