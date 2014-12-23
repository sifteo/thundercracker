/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
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
#include "llvm/Support/raw_ostream.h"

#ifdef WIN32
#  include <io.h>
#  include <fcntl.h>
#else
#  include <unistd.h>
#endif


namespace llvm {


static void console_color(raw_ostream &OS, unsigned code)
{
#ifndef WIN32
    if (isatty(2))
        OS << "\e[" << code << "m";
#endif
}

static void message_handler(const std::string &prefix, const std::string &err)
{
    /*
     * Blast the whole error to stderr.
     */

    SmallVector<char, 64> Buffer;
    raw_svector_ostream OS(Buffer);

    OS << "-!- ";
    console_color(OS, 1);   // bright
    console_color(OS, 32);  // green
    OS << "Slinky";
    console_color(OS, 0);   // normal
    OS << " ";
    console_color(OS, 1);   // bright
    OS << prefix << ":";
    console_color(OS, 0);   // normal
    OS << " " << err << "\n";

    StringRef MessageStr = OS.str();

    ssize_t result = ::write(2, MessageStr.data(), MessageStr.size());
    if (result <= 0) {
        // Some versions of glibc force us to check the return value...
        assert(0);
    }
}

static void fatal_error_handler(void *, const std::string &err)
{
    message_handler("error", err);
}

static struct ErrorHandlerSetup {
    ErrorHandlerSetup() {
        install_fatal_error_handler(fatal_error_handler, NULL);
    }
} gErrorHandlerSetup;

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

void report_warning(const Twine &msg)
{
    message_handler("warning", msg.str());
}

void report_warning(const char *msg)
{
    Twine T(msg);
    report_warning(T);
}

void report_warning(const Function *F, const Twine &msg)
{
    std::string FuncName = F->getName().str();
    demangle(FuncName);
    report_warning("In " + Twine(FuncName) + ": " + msg);
}

void report_warning(const Function *F, const char *msg)
{
    Twine T(msg);
    report_warning(F, T);
}


} // end namespace

#endif
