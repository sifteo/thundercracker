/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

namespace llvm {

    class MachineInstr;

    class SVMBlockSizeAccumulator {
    public:
        void clear();
        unsigned getByteCount() const;

        void AddInstr(const MachineInstr *MI);
        void InstrAlign(unsigned A);

    private:
        unsigned InstrSizeTotal;
        unsigned ConstSizeTotal;
        int ConstAlignment; 
        SmallSet<unsigned, 128> UsedCPI;
    };

}

#endif
