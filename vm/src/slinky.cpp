/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM.
 * It is based on the LLVM "llc", "opt", and "llvm-link" tools.
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

//===-- slinky.cpp - Sifteo Linker / Code Generator frontend --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/Linker.h"
#include "llvm/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/ADT/Triple.h"
#include "llvm/Support/IRReader.h"
#include "llvm/CodeGen/LinkAllAsmWriterComponents.h"
#include "llvm/CodeGen/LinkAllCodegenComponents.h"
#include "llvm/Config/config.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PluginLoader.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/IPO.h"
#include <memory>
using namespace llvm;

extern "C" void LLVMInitializeSVMAsmPrinter();
extern "C" void LLVMInitializeSVMTarget();
extern "C" void LLVMInitializeSVMTargetMC();
extern "C" void LLVMInitializeSVMTargetInfo();

namespace llvm {
    ModulePass *createInlineGlobalCtorsPass();
    BasicBlockPass *createEarlyLTIPass();
    BasicBlockPass *createLateLTIPass();
}

static const char HelpText[] =
    "\n"
    "    slinky \\/\\/\\ Sifteo Linker and Code Generator\n"
    "\n"
    "    This tool is a combination linker, whole-program optimizer, and\n"
    "    code generator. It converts one or more LLVM object files (.o) to\n"
    "    a single fully-linked Sifteo VM executable, in ELF format.\n"
    "\n"
    "    This tool takes many of the standard LLVM code generation and\n"
    "    diagnostic command line options, detailed below.\n"
    ;
static cl::extrahelp LicenseText(
    "\n"
    "LICENSE:\n"
    "\n"
    "    The Sifteo Linker tool and its code generation backend are\n"
    "    Copyright (c) 2012 Sifteo, Inc. All rights reserved.\n"
    "\n"
    "    Based on the open source LLVM project. Portions \n"
    "    Copyright (c) 2003-2012 University of Illinois at Urbana-Champaign.\n"
    "    All rights reserved.\n"
    "\n"
    "    Contains code that was originally released under the Univeristy of\n"
    "    Illinois/NCSA Open Source License, reproduced below.\n"
    "\n"
    "OPEN SOURCE LICENSE:\n"
    "\n"
    "    Permission is hereby granted, free of charge, to any person obtaining a copy of\n"
    "    this software and associated documentation files (the \"Software\"), to deal with\n"
    "    the Software without restriction, including without limitation the rights to\n"
    "    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies\n"
    "    of the Software, and to permit persons to whom the Software is furnished to do\n"
    "    so, subject to the following conditions:\n"
    "\n"
    "        * Redistributions of source code must retain the above copyright notice,\n"
    "          this list of conditions and the following disclaimers.\n"
    "\n"
    "        * Redistributions in binary form must reproduce the above copyright notice,\n"
    "          this list of conditions and the following disclaimers in the\n"
    "          documentation and/or other materials provided with the distribution.\n"
    "\n"
    "        * Neither the names of the LLVM Team, University of Illinois at\n"
    "          Urbana-Champaign, nor the names of its contributors may be used to\n"
    "          endorse or promote products derived from this Software without specific\n"
    "          prior written permission.\n"
    "\n"
    "    THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n"
    "    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS\n"
    "    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE\n"
    "    CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n"
    "    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n"
    "    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE\n"
    "    SOFTWARE.\n"
    "\n");

static cl::list<std::string>
InputFilenames(cl::Positional, cl::OneOrMore,
    cl::desc("<input bitcode files>"));

static cl::opt<std::string>
OutputFilename("o", cl::desc("Output filename"), cl::value_desc("filename"));

static cl::opt<bool>
Verbose("v", cl::desc("Print information about actions taken"));

static cl::opt<bool>
DisableInline("disable-inlining", cl::desc("Do not run the inliner pass"));

// Determine optimization level.
static cl::opt<char>
OptLevel("O",
         cl::desc("Optimization level. [-O0, -O1, -O2, or -O3] "
                  "(default = '-O2')"),
         cl::Prefix,
         cl::ZeroOrMore,
         cl::init(' '));

cl::opt<bool> AsmOutput("asm",
    cl::desc("Emit VM assembly output"));

cl::opt<bool> NoVerify("disable-verify", cl::Hidden,
    cl::desc("Do not verify input module"));


static std::auto_ptr<Module> LoadFile(const char *argv0,
    const std::string &FN, LLVMContext& Context)
{
    sys::Path Filename;
    if (!Filename.set(FN)) {
        errs() << "Invalid file name: '" << FN << "'\n";
        return std::auto_ptr<Module>();
    }

    SMDiagnostic Err;
    if (Verbose)
        errs() << "Loading '" << Filename.c_str() << "'\n";
    Module* Result = 0;
  
    const std::string &FNStr = Filename.str();
    Result = ParseIRFile(FNStr, Err, Context);
    if (Result)
        return std::auto_ptr<Module>(Result);   // Load successful!

    Err.Print(argv0, errs());
    return std::auto_ptr<Module>();
}

static std::auto_ptr<Module> LoadInputs(const char *argv0, LLVMContext &Context)
{
    unsigned BaseArg = 0;
    std::string ErrorMessage;

    std::auto_ptr<Module> Composite(LoadFile(argv0, InputFilenames[BaseArg], Context));
    if (Composite.get() == 0) {
        errs() << argv0 << ": error loading file '"
             << InputFilenames[BaseArg] << "'\n";
        return std::auto_ptr<Module>();
    }

    for (unsigned i = BaseArg+1; i < InputFilenames.size(); ++i) {
        std::auto_ptr<Module> M(LoadFile(argv0, InputFilenames[i], Context));
    
        if (M.get() == 0) {
            errs() << argv0 << ": error loading file '" <<InputFilenames[i]<< "'\n";
            return std::auto_ptr<Module>();
        }

        if (Verbose)
            errs() << "Linking in '" << InputFilenames[i] << "'\n";

        if (Linker::LinkModules(Composite.get(), M.get(),
            Linker::DestroySource, &ErrorMessage)) {
            errs() << argv0 << ": link error in '" << InputFilenames[i]
                << "': " << ErrorMessage << "\n";
            return std::auto_ptr<Module>();
        }
    }

    return Composite;
}

static tool_output_file *GetOutputStream()
{
    if (OutputFilename.empty())
        OutputFilename = AsmOutput ? "program.s" : "program.elf";

    std::string error;
    unsigned OpenFlags = AsmOutput ? 0 : raw_fd_ostream::F_Binary;
    tool_output_file *FDOut =
        new tool_output_file(OutputFilename.c_str(), error, OpenFlags);
    if (!error.empty()) {
        errs() << error << '\n';
        delete FDOut;
        return 0;
    }

    return FDOut;
}

static void AddOptimizationPasses(PassManagerBase &MPM,
    FunctionPassManager &FPM, unsigned OptLevel)
{
    PassManagerBuilder Builder;
    Builder.OptLevel = OptLevel;

    if (DisableInline) {
        // No inlining pass
    } else if (OptLevel > 1) {
        unsigned Threshold = 225;
        if (OptLevel > 2)
            Threshold = 275;
        Builder.Inliner = createFunctionInliningPass(Threshold);
    } else {
        Builder.Inliner = createAlwaysInlinerPass();
    }

    Builder.DisableUnitAtATime = false;
    Builder.DisableUnrollLoops = OptLevel == 0;
    Builder.DisableSimplifyLibCalls = false;
  
    Builder.populateFunctionPassManager(FPM);
    Builder.populateModulePassManager(MPM);
}

static void AddStandardLinkPasses(PassManagerBase &PM, unsigned OptLevel)
{
    if (OptLevel > 0) {
        PassManagerBuilder Builder;
        Builder.populateLTOPassManager(PM,
            /*Internalize=*/ true,
            /*RunInliner=*/ !DisableInline);
    }
}

// Custom passes that occur before all link-time optimization
static void AddEarlyPasses(PassManagerBase &PM)
{
    // Early LTI expansion handles things likely to result in dead code
    // elimination and constant folding.
    PM.add(createEarlyLTIPass());
}

// Custom passes that occur between two rounds of optimization.
static void AddMiddlePasses(PassManagerBase &PM)
{
    // Do late LTI expansion. This expects to have at least one round of
    // optimization run since early LTI expansion has occurred.
    PM.add(createLateLTIPass());

    // After IPO, which may completely eliminate constructors, convert
    // any remaining constructors into calls at the top of main().
    // This may result in additional inlining and dead code elimination.
    PM.add(createInlineGlobalCtorsPass());
}

int main(int argc, char **argv)
{
    sys::PrintStackTraceOnErrorSignal();
    PrettyStackTraceProgram X(argc, argv);

    // Enable debug stream buffering.
    EnableDebugBuffering = true;

    LLVMContext &Context = getGlobalContext();
    llvm_shutdown_obj Y;  // Call llvm_shutdown() on exit.

    // Initialize core passes
    PassRegistry &Registry = *PassRegistry::getPassRegistry();
    initializeCore(Registry);
    initializeScalarOpts(Registry);
    initializeIPO(Registry);
    initializeAnalysis(Registry);
    initializeIPA(Registry);
    initializeTransformUtils(Registry);
    initializeInstCombine(Registry);
    initializeTarget(Registry);

    // Initialize targets first, so that --version shows registered targets.
    LLVMInitializeSVMAsmPrinter();
    LLVMInitializeSVMTarget();
    LLVMInitializeSVMTargetMC();
    LLVMInitializeSVMTargetInfo();

    cl::ParseCommandLineOptions(argc, argv, HelpText);
    
    // Load and link the input modules
    std::auto_ptr<Module> Composite = LoadInputs(argv[0], Context);
    if (!Composite.get())
        return 1;
    Module &mod = *Composite.get();

    // Always override the target triple
    mod.setTargetTriple(Triple::normalize("thumb-sifteo-vm"));
    Triple TheTriple(mod.getTargetTriple());

    // Allocate target machine
    const Target *TheTarget = 0;
    {
        std::string Err;
        TheTarget = TargetRegistry::lookupTarget(TheTriple.getTriple(), Err);
        assert(TheTarget != 0);
        if (TheTarget == 0) {
            errs() << argv[0] << ": error selecting target\n";
            return 1;
        }
    }

    std::auto_ptr<TargetMachine>
        target(TheTarget->createTargetMachine(TheTriple.getTriple(),
            "", "", Reloc::Static, CodeModel::Default));
    assert(target.get() && "Could not allocate target machine!");
    TargetMachine &Target = *target.get();

    CodeGenOpt::Level OLvl = CodeGenOpt::Default;
    switch (OptLevel) {
    default:
        errs() << argv[0] << ": invalid optimization level.\n";
        return 1;
    case ' ': break;
    case '0': OLvl = CodeGenOpt::None; break;
    case '1': OLvl = CodeGenOpt::Less; break;
    case '2': OLvl = CodeGenOpt::Default; break;
    case '3': OLvl = CodeGenOpt::Aggressive; break;
    }

    // Build passes
    PassManager PM;
    FunctionPassManager FPM(&mod);
    FPM.add(new TargetData(*Target.getTargetData()));

    // Link-time optimization
    AddEarlyPasses(PM);
    AddStandardLinkPasses(PM, OLvl);

    // Additional optimizations, both before and after our custom 'middle' passes
    AddOptimizationPasses(PM, FPM, OLvl);
    AddMiddlePasses(PM);
    AddOptimizationPasses(PM, FPM, OLvl);

    // Override default to generate verbose assembly.
    Target.setAsmVerbosityDefault(true);

    // Target passes
    PM.add(new TargetData(*Target.getTargetData()));
    OwningPtr<tool_output_file> Out(GetOutputStream());
    if (!Out)
        return 1;
    TargetMachine::CodeGenFileType FileType = AsmOutput ?
        TargetMachine::CGFT_AssemblyFile : TargetMachine::CGFT_ObjectFile;
    formatted_raw_ostream FOS(Out->os());
    Target.addPassesToEmitFile(PM, FOS, FileType, OLvl, NoVerify);

    // Before executing passes, print the final values of the LLVM options.
    cl::PrintOptionValues();
    
    // Run function passes
    FPM.doInitialization();
    for (Module::iterator F = mod.begin(), E = mod.end(); F != E; ++F)
        FPM.run(*F);
    FPM.doFinalization();

    // Run module passes
    PM.run(mod);

    // Success!
    Out->keep();

    return 0;
}
