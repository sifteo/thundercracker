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
 
#ifndef SVM_SYMBOLDECORATION_H
#define SVM_SYMBOLDECORATION_H

#include "llvm/ADT/Twine.h"

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
        bool isLongBranch;
        bool isSys;
        bool isMeta;
        unsigned sysNumber;
        int32_t metaKey;
        int32_t offset;

        static const char SYS[];
        static const char CALL[];
        static const char TCALL[];
        static const char LB[];
        static const char OFFSET[];
        static const char META[];
        static const char INIT[];
        static const char SEPARATOR[];

        void Init();
        StringRef Decode(StringRef Name);
        static Constant *Apply(Module *M, const GlobalValue *Value, Twine Prefix);
        static Constant *ApplyOffset(Module *M, const GlobalValue *Value, int32_t offset);

    private:
        static bool testAndStripPrefix(StringRef &Name, StringRef Prefix);
        static bool testAndStripNumberedPrefix(StringRef &Name, StringRef Prefix, int32_t &num);
    };

}  // end namespace

#endif
