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

#include "SVMMCTargetDesc.h"
#include "SVMMCAsmInfo.h"
#include "SVMInstPrinter.h"
#include "llvm/MC/MCCodeGenInfo.h"
#include "llvm/MC/MCInstrInfo.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Support/TargetRegistry.h"

#define GET_INSTRINFO_MC_DESC
#include "SVMGenInstrInfo.inc"

#define GET_REGINFO_MC_DESC
#include "SVMGenRegisterInfo.inc"

using namespace llvm;

static MCInstrInfo *createSVMMCInstrInfo()
{
    MCInstrInfo *X = new MCInstrInfo();
    InitSVMMCInstrInfo(X);
    return X;
}

static MCRegisterInfo *createSVMMCRegisterInfo(StringRef TT)
{
    MCRegisterInfo *X = new MCRegisterInfo();
    InitSVMMCRegisterInfo(X, SVM::R7);
    return X;
}

static MCCodeGenInfo *createSVMMCCodeGenInfo(StringRef TT, Reloc::Model RM, CodeModel::Model CM)
{
    MCCodeGenInfo *X = new MCCodeGenInfo();
    X->InitMCCodeGenInfo(RM, CM);
    return X;
}

static MCStreamer *createMCStreamer(const Target &T, StringRef TT,
                                    MCContext &Ctx, MCAsmBackend &MAB,
                                    raw_ostream &_OS,
                                    MCCodeEmitter *_Emitter,
                                    bool RelaxAll,
                                    bool NoExecStack)
{
    Triple TheTriple(TT);
    return createELFStreamer(Ctx, MAB, _OS, _Emitter, RelaxAll, NoExecStack);
}

static MCInstPrinter *createSVMMCInstPrinter(const Target &T, unsigned SyntaxVariant,
                                             const MCAsmInfo &MAI, const MCSubtargetInfo &STI)
{
    return new SVMInstPrinter(MAI);
}

extern "C" void LLVMInitializeSVMTargetMC() {
    RegisterMCAsmInfo<SVMMCAsmInfo> X(TheSVMTarget);
    TargetRegistry::RegisterMCCodeGenInfo(TheSVMTarget, createSVMMCCodeGenInfo);
    TargetRegistry::RegisterMCInstrInfo(TheSVMTarget, createSVMMCInstrInfo);
    TargetRegistry::RegisterMCRegInfo(TheSVMTarget, createSVMMCRegisterInfo);
    TargetRegistry::RegisterMCCodeEmitter(TheSVMTarget, createSVMMCCodeEmitter);
    TargetRegistry::RegisterMCAsmBackend(TheSVMTarget, createSVMAsmBackend);
    TargetRegistry::RegisterMCObjectStreamer(TheSVMTarget, createMCStreamer);
    TargetRegistry::RegisterMCInstPrinter(TheSVMTarget, createSVMMCInstPrinter);
}
