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

#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "llvm/ADT/SmallVector.h"
#include <utility>
#include <map>
using namespace llvm;

namespace llvm {

class CallSite;
class Instruction;

void initializeUUIDGeneratorPass(PassRegistry&);

class UUIDGenerator : public ModulePass {
public:
    static char ID;
    UUIDGenerator() : ModulePass(ID) {
        initializeUUIDGeneratorPass(*PassRegistry::getPassRegistry());
    }

    virtual bool runOnModule(Module &M);

    virtual const char *getPassName() const {
        return "UUID generator pass";
    }

    void getAnalysisUsage(AnalysisUsage &AU) const {
        AU.setPreservesAll();
    }

    uint32_t getValueFor(CallSite &CS) const;

private:
    struct UUID_t {
        union {
            struct {
                uint32_t time_low;
                uint16_t time_mid;
                uint16_t time_hi_and_version;
                uint8_t clk_seq_hi_res;
                uint8_t clk_seq_low;
                uint8_t node[6];
            };
            uint8_t  bytes[16];
            uint16_t hwords[8];
            uint32_t words[4];
        };
    };

    struct Args {
        unsigned key;
        unsigned index;
    };

    typedef std::map<unsigned, UUID_t> ValueMap_t;
    ValueMap_t ValueMap;

    static void argUnpack(CallSite &CS, Args &a);

    void runOnFunction(Function &F);
    void runOnBasicBlock(BasicBlock &BB);
    void runOnCall(CallSite &CS);

    static UUID_t generate();
};

}  // end namespace llvm
