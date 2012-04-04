/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_ERRORREPORTER_H
#define SVM_ERRORREPORTER_H

#include <cxxabi.h>
#include "ErrorReporter.h"
#include "llvm/Function.h"
#include "llvm/Instruction.h"
#include "llvm/BasicBlock.h"
#include "llvm/Support/DebugLoc.h"
#include "llvm/ADT/Twine.h"
#include "llvm/Analysis/DebugInfo.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/CodeGen/SelectionDAG.h"
#include "llvm/CodeGen/MachineFunction.h"

namespace llvm {

static void demangle(std::string &name)
{
    // This uses the demangler built into GCC's libstdc++.
    // It uses the same name mangling style as clang.

    int status;
    char *result = abi::__cxa_demangle(name.c_str(), 0, 0, &status);
    if (status == 0) {
        name = result;
        free(result);
    }
}

void report_fatal_error(const Instruction *I, const Twine &msg)
{
    LLVMContext &Ctx = I->getContext();
    DebugLoc dl = I->getDebugLoc();
    const BasicBlock *BB = I->getParent();
    const Function *Func = BB->getParent();
    DISubprogram SP = getDISubprogram(dl.getScope(Ctx));
    std::string FuncName = Func->getName().str();
    Twine lineInfo;
    
    demangle(FuncName);

    if (dl.isUnknown())
        lineInfo = "(unknown line)";
    else
        lineInfo = Twine(SP.getFilename()) + ":" +
            Twine(dl.getLine()) + "," + Twine(dl.getCol());

    report_fatal_error("In " + Twine(FuncName) + " at " + lineInfo + ": " + msg);
}

void report_fatal_error(const Instruction *I, const char *msg)
{
    Twine T(msg);
    report_fatal_error(I, T);
}

void report_fatal_error(const SDNode *N, SelectionDAG &DAG, const Twine &msg)
{
    LLVMContext &Ctx = *DAG.getContext();
    DebugLoc dl = N->getDebugLoc();
    const Function *Func = DAG.getMachineFunction().getFunction();
    DISubprogram SP = getDISubprogram(dl.getScope(Ctx));
    std::string FuncName = Func->getName().str();
    Twine lineInfo;
    
    demangle(FuncName);

    if (dl.isUnknown())
        lineInfo = "(unknown line)";
    else
        lineInfo = Twine(SP.getFilename()) + ":" +
            Twine(dl.getLine()) + "," + Twine(dl.getCol());

    report_fatal_error("In " + Twine(FuncName) + " at " + lineInfo + ": " + msg);
}

void report_fatal_error(const SDNode *N, SelectionDAG &DAG, const char *msg)
{
    Twine T(msg);
    report_fatal_error(N, DAG, T);
}


} // end namespace

#endif
