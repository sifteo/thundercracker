/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This file implements MetadataTransform(), which converts a
 * _SYS_lti_metadata() call into a global object in the ".metadata" segment.
 */

#include "ErrorReporter.h"
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

    if (CS.arg_size() < 1)
        report_fatal_error(I, "_SYS_lti_metadata requires at least one arg");

    // Parse the 'key' parameter
    ConstantInt *CI = dyn_cast<ConstantInt>(CS.getArgument(0));
    if (!CI)
        report_fatal_error(I, "Metadata key must be a constant integer.");
    uint16_t key = CI->getZExtValue();
    if (key != CI->getZExtValue())
        report_fatal_error(I, "Metadata key argument is too large.");

    /*
     * Parse every other parameter, ensuring its const-ness, transforming
     * it if necessary, and packing it into an anonymous struct value.
     */

    SmallVector<Constant*, 8> Members;
    unsigned align = 1;

    for (unsigned idx = 1, E = CS.arg_size(); idx != E; ++idx) {
        Value *Arg = CS.getArgument(idx);

        // First look for a C-style string
        std::string str;
        if (GetConstantStringInfo(Arg, str)) {
            Members.push_back(ConstantArray::get(Ctx, str, true));
            continue;
        }

        // Now, any other constant value
        Constant *C = dyn_cast<Constant>(Arg);
        if (C) {
            align = std::max(align, TD->getABITypeAlignment(C->getType()));
            Members.push_back(C);
            continue;
        }

        report_fatal_error(I, "Argument " + Twine(idx + 1) +
            " to _SYS_lti_metadata is not constant.");
    }

    Constant *Struct = ConstantStruct::getAnon(Members);

    /*
     * Install this metadata item as a global variable in a special section
     */

    GlobalVariable *GV = new GlobalVariable(*M, Struct->getType(),
        true, GlobalValue::ExternalLinkage, Struct, "", 0, false);

    GV->setAlignment(align);
    GV->setName("_meta$" + Twine(key));
    GV->setSection(".metadata");

    // Remove the original _SYS_lti_metadata() call
    I->eraseFromParent();
}

}   // end namespace llvm

