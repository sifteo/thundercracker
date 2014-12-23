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
 * The SVMBlockSizeAccumulator is a utility object that can accurately
 * measure the size of a flash block containing a particular mix of
 * instructions and data.
 *
 * It is intended to help answer the question: "If I add this to a block,
 * will the block overflow?"
 */

#ifndef SVM_BLOCKSIZEACCUMULATOR_H
#define SVM_BLOCKSIZEACCUMULATOR_H

#include "llvm/ADT/SmallSet.h"
#include "llvm/Support/raw_ostream.h"

namespace llvm {

    class TargetData;
    class MachineInstr;
    class MachineConstantPoolEntry;

    class SVMBlockSizeAccumulator {
    public:
        void clear();
        unsigned getByteCount() const;
        
        void describe(raw_ostream &OS);

        void AddInstr(const MachineInstr *MI);
        void AddInstr(unsigned bytes);
        void AddInstrPrefix(unsigned bytes);
        void AddInstrSuffix(unsigned bytes);
        void AddConstantsForInstr(const MachineInstr *MI);
        void AddConstant(const TargetData &TD, const MachineConstantPoolEntry &CPE);
        void AddConstant(unsigned bytes, unsigned align=1);
        void InstrAlign(unsigned A);

    private:
        unsigned InstrSizeTotal;
        unsigned InstrPrefixTotal;
        unsigned InstrSuffixTotal;
        unsigned ConstSizeTotal;
        unsigned ConstAlignment; 
        SmallSet<unsigned, 128> UsedCPI;
    };

}

#endif
