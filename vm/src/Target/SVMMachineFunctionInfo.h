/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MACHINEFUNCTIONINFO_H
#define SVM_MACHINEFUNCTIONINFO_H

#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

    class SVMMachineFunctionInfo : public MachineFunctionInfo {
    public:
        SVMMachineFunctionInfo()
            : GlobalBaseReg(0), VarArgsFrameOffset(0), SRetReturnReg(0) {}
        explicit SVMMachineFunctionInfo(MachineFunction &MF)
            : GlobalBaseReg(0), VarArgsFrameOffset(0), SRetReturnReg(0) {}

    };

}

#endif
