/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svm.h"
#include "svmruntime.h"
#include "svmmemory.h"
#include "system.h"
#include "system_mc.h"

using namespace Svm;

SvmMemory::PhysAddr SvmRuntime::topOfStackPA;
SvmMemory::PhysAddr SvmRuntime::stackLowWaterMark;


/*
 * If enabled, monitor stack usage and print when we
 * have a new low water mark.
 */
void SvmRuntime::onStackModification(SvmMemory::PhysAddr sp)
{
    if (SystemMC::getSystem()->opt_svmStackMonitor && sp < stackLowWaterMark) {
        stackLowWaterMark = sp;
        LOG(("SVM: New stack low water mark, 0x%p (%d bytes)\n",
             reinterpret_cast<void*>(stackLowWaterMark), int(topOfStackPA - stackLowWaterMark)));
    }
}
