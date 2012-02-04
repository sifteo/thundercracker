/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */
 
#ifndef SVM_SYMBOLDECORATION_H
#define SVM_SYMBOLDECORATION_H

/*
 * Magic symbol prefixes.
 *
 * These are used as an ad-hoc way of passing annotations to the
 * "linker" in SVMELFProgramWriter. We have some very nonstandard
 * features in the SVM architecture: syscalls that look like functions,
 * function calls and tail calls that encode the instruction op into
 * the target pointer, etc.
 *
 * Since LLVM has no native way to represent these types cleanly, as
 * far as I'm aware, this approach lets us pass custom info to our
 * linker without patching the LLVM core.
 */

namespace llvm {

    class StringRef;
    class Constant;
    class Module;
    class GlobalValue;
    class MCContext;
    class MCSymbol;

    struct SVMDecorations {
        // Attributes of a decorated symbol
        bool isCall;
        bool isTailCall;
        bool isSys;
        unsigned sysNumber;

        static const char SYS[];
        static const char CALL[];
        static const char TCALL[];
        
        StringRef Decode(StringRef Name);
        static Constant *Apply(Module *M, const GlobalValue *Value, StringRef Prefix);

    private:
        static bool testAndStripPrefix(StringRef &Name, StringRef Prefix);
    };

    struct SVMEntryPoint {
        // Compatible entry point names

        static bool isEntry(StringRef Name);
        static MCSymbol *findEntry(const MCContext &Ctx);
        static StringRef getPreferredSignature();

    private:
        static const char *nameTable[];
    };

}  // end namespace

#endif
