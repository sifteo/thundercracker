/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * Micah Elizabeth Scott <micah@misc.name>
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef SVM_TARGETMACHINE_H
#define SVM_TARGETMACHINE_H

#include "SVMInstrInfo.h"
#include "SVMISelLowering.h"
#include "SVMFrameLowering.h"
#include "SVMSelectionDAGInfo.h"
#include "SVMSubtarget.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetFrameLowering.h"

namespace llvm {

class SVMTargetMachine : public LLVMTargetMachine {
    const TargetData DataLayout;
    SVMTargetLowering TLInfo;
    SVMSelectionDAGInfo TSInfo;
    SVMInstrInfo InstrInfo;
    SVMFrameLowering FrameLowering;
    SVMSubtarget Subtarget;

public:
    SVMTargetMachine(const Target &T, StringRef TT, 
                     StringRef CPU, StringRef FS,
                     Reloc::Model RM, CodeModel::Model CM);

    static uint32_t getBlockSize();
    static uint32_t getBundleSize();
    static uint32_t getFlashBase();
    static uint32_t getRAMBase();
    static uint32_t getRAMSize();
    static uint8_t getPaddingByte();
    static const char *getDataLayoutString();
    static bool isTargetCompatible(LLVMContext& Context, const TargetData &TD);

    virtual const SVMInstrInfo *getInstrInfo() const { return &InstrInfo; }
    virtual const TargetFrameLowering  *getFrameLowering() const { return &FrameLowering; }
    virtual const SVMRegisterInfo *getRegisterInfo() const { return &InstrInfo.getRegisterInfo(); }
    virtual const SVMTargetLowering* getTargetLowering() const { return &TLInfo; }
    virtual const SVMSelectionDAGInfo* getSelectionDAGInfo() const { return &TSInfo; }
    virtual const TargetData *getTargetData() const { return &DataLayout; }
    virtual const SVMSubtarget *getSubtargetImpl() const { return &Subtarget; }

    virtual bool addInstSelector(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
    virtual bool addPreEmitPass(PassManagerBase &PM, CodeGenOpt::Level OptLevel);
};

}

#endif
