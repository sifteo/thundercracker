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
using namespace Svm;

class SvmDebugger {
public:
    SvmDebugger();  // Do not implement
    
    static void handleBreakpoint(void *param=0);
};

#endif // SVM_DEBUGGER_H
