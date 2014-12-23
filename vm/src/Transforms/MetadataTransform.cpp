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

/*
 * This file implements MetadataTransform(), which converts a
 * _SYS_lti_metadata() call into a global object in the ".metadata" segment.
 */

#include "Support/ErrorReporter.h"
#include "Target/SVMSymbolDecoration.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instruction.h"
#include "llvm/Instructions.h"
#include "llvm/Support/CallSite.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Target/TargetData.h"

namespace llvm {

void MetadataTransform(CallSite &CS, const TargetData *TD)
{
    Instruction *I = CS.getInstruction();
    Module *M = I->getParent()->getParent()->getParent();
    LLVMContext &Ctx = I->getContext();
    unsigned argIdx = 0;

    if (CS.arg_size() < 2)
        report_fatal_error(I, "_SYS_lti_metadata requires at least two args");

    // Parse the 'key' parameter
    ConstantInt *CI = dyn_cast<ConstantInt>(CS.getArgument(argIdx++));
    if (!CI)
        report_fatal_error(I, "Metadata key must be a constant integer.");
    uint16_t key = CI->getZExtValue();
    if (key != CI->getZExtValue())
        report_fatal_error(I, "Metadata key argument is too large.");

    // Parse the 'fmt' parameter
    std::string fmt;
    if (!GetConstantStringInfo(CS.getArgument(argIdx++), fmt))
        report_fatal_error(I, "Metadata format must be a constant string.");
    if (fmt.size() != CS.arg_size() - 2)
        report_fatal_error(I, "Length of metadata format must match number of parameters");
    if (fmt.size() == 0)
        report_fatal_error(I, "Empty metadata values are not supported");

    /*
     * Parse every other parameter according to the format string
     */

    SmallVector<Constant*, 8> Members;
    unsigned align = 1;

    for (std::string::iterator FI = fmt.begin(), FE = fmt.end();
        FI != FE; ++FI, argIdx++) {
        Constant *Arg = dyn_cast<Constant>(CS.getArgument(argIdx));
        Constant *C;
        if (!Arg)
            report_fatal_error(I, "Metadata argument " + Twine(argIdx+1) + " is not constant");

        /*
         * First, non-integer types
         */

        switch (*FI) {
            case 's': {
                std::string str;
                if (!GetConstantStringInfo(Arg, str))
                    report_fatal_error(I, "Metadata formatter 's' requires a constant string");
                Members.push_back(ConstantArray::get(Ctx, str, false));
                continue;
            }
        }
        
        /*
         * Integer types
         */

        if (Arg->getType()->isPointerTy())
            Arg = ConstantExpr::getPointerCast(Arg, Type::getInt32Ty(Ctx));

        if (!Arg->getType()->isIntegerTy())
            report_fatal_error(I, "Metadata argument " + Twine(argIdx+1) +
                " can't be converted to an integer type.");

        switch (*FI) {

            case 'b':
                C = ConstantExpr::getIntegerCast(Arg, Type::getInt8Ty(Ctx), true);
                break;

            case 'B':
                C = ConstantExpr::getIntegerCast(Arg, Type::getInt8Ty(Ctx), false);
                break;

            case 'h':
                C = ConstantExpr::getIntegerCast(Arg, Type::getInt16Ty(Ctx), true);
                break;

            case 'H':
                C = ConstantExpr::getIntegerCast(Arg, Type::getInt16Ty(Ctx), false);
                break;

            case 'i':
                C = ConstantExpr::getIntegerCast(Arg, Type::getInt32Ty(Ctx), true);
                break;

            case 'I':
                C = ConstantExpr::getIntegerCast(Arg, Type::getInt32Ty(Ctx), false);
                break;

            default:
                report_fatal_error(I, "Unsupported format character '" + Twine(*FI) + "' in metadata");
        }

        align = std::max(align, TD->getABITypeAlignment(C->getType()));
        Members.push_back(C);
    }

    Constant *Struct = ConstantStruct::getAnon(Members);

    /*
     * Install this metadata item as a global variable in a special section
     */

    GlobalVariable *GV = new GlobalVariable(*M, Struct->getType(),
        true, GlobalValue::ExternalLinkage, Struct, "", 0, false);

    GV->setAlignment(align);
    GV->setName(SVMDecorations::META + Twine(key) + SVMDecorations::SEPARATOR);
    GV->setSection(".metadata");

    // Remove the original _SYS_lti_metadata() call
    I->eraseFromParent();
}

}   // end namespace llvm

