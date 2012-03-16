/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * During LateLTI, we convert _SYS_lti_metadata() calls into packed
 * metadata value records. Each key/value pair ends up as a GlobalValue in
 * the ".metadata" section, with a name of the form "_meta$<key>" and an
 * anonymous struct containing the value.
 *
 * This pass gathers all such values into a single packed struct containing
 * both the key array and the subsequent values. We need to both respect
 * the alignment of individual values, and arrange them to prevent crossing
 * flash block boundaries.
 */

#include "SVMSymbolDecoration.h"
#include "SVMMemoryLayout.h"
#include "SVMTargetMachine.h"
#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/LLVMContext.h"
#include "llvm/Instructions.h"
#include "llvm/Attributes.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Target/TargetData.h"
using namespace llvm;

namespace llvm {
    ModulePass *createMetadataCollectorPass();
}

namespace {

    class MetadataValues {
    public:
        void append(Constant *C) {
            ValueList.push_back(C);
        }

        void finalize(const TargetData *TD) {
            ValueStruct = ConstantStruct::getAnon(ValueList);
            size = TD->getTypeAllocSize(ValueStruct->getType());
            align = TD->getABITypeAlignment(ValueStruct->getType());
        }

        unsigned getSize() const {
            return size;
        }

        unsigned getAddress() const {
            return address;
        }
        
        void setAddress(unsigned a) {
            address = a;
        }
        
        Constant *getStruct() const {
            return ValueStruct;
        }

        unsigned findFirstAddress(unsigned base) {
            uint32_t blockSize = SVMTargetMachine::getBlockSize();
            assert(size <= blockSize);
            base = RoundUpToAlignment(base, align);

            if (base / blockSize != (base + size - 1) / blockSize)
                base = RoundUpToAlignment(base, blockSize);
            assert(base / blockSize == (base + size - 1) / blockSize);
        
            return base;
        }

    private:
        typedef SmallVector<Constant*, 4> ValueList_t;
        ValueList_t ValueList;
        Constant *ValueStruct;
        unsigned size;
        unsigned align;
        unsigned address;
    };
    
    class MetadataCollectorPass : public ModulePass {
    public:
        static char ID;
        MetadataCollectorPass()
            : ModulePass(ID) {}
        
        virtual bool runOnModule(Module &M);

        virtual const char *getPassName() const {
            return "Collecting metadata items";
        }

    private:
        typedef DenseMap<unsigned, MetadataValues> Dict_t;
        typedef SmallVector<unsigned, 16> Layout_t;

        Dict_t Dict;
        Layout_t Layout;

        void collectValues(Module &M);
        void finalizeAll(Module &M, const TargetData *TD);
        void createLayout();
        void pack(Module &M);
    };

}

char MetadataCollectorPass::ID = 0;

bool MetadataCollectorPass::runOnModule(Module &M)
{
    const TargetData *TD = getAnalysisIfAvailable<TargetData>();
    assert(TD);
    assert(Dict.empty());
    assert(Layout.empty());

    collectValues(M);
    finalizeAll(M, TD);
    createLayout();
    pack(M);

    Dict.clear();
    Layout.clear();
    return true;
}

void MetadataCollectorPass::collectValues(Module &M)
{
    /*
     * Sift through the module's global variables, and extract any that
     * are in the .metadata section. Insert them into our Dict.
     */
    
    for (Module::global_iterator I = M.global_begin(), E = M.global_end(); I != E;) {
        if (I->getSection() != ".metadata") {
            ++I;
            continue;
        }

        StringRef Name = I->getName();
        GlobalVariable *GV = dyn_cast<GlobalVariable>(&*I);
        if (!GV)
            report_fatal_error("Metadata item " + Twine(Name)
                + " is not a GlobalVariable");

        if (!GV->isConstant())
            report_fatal_error("Metadata item " + Twine(Name)
                + " is not constant");

        SVMDecorations Deco;
        Deco.Decode(Name);
        if (!Deco.isMeta)
            report_fatal_error("Metadata item " + Twine(Name)
                + " does not have a properly formatted key");
        
        Dict[Deco.metaKey].append(GV->getInitializer());

        ++I;
        GV->eraseFromParent();
    }
}

void MetadataCollectorPass::finalizeAll(Module &M, const TargetData *TD)
{
    /*
     * Calculate sizes for all metadata values, and assemble them into
     * anonymous structures.
     */
    
    for (Dict_t::iterator I = Dict.begin(), E = Dict.end(); I != E; ++I) {
        I->second.finalize(TD);

        if (I->second.getSize() > SVMTargetMachine::getBlockSize())
            report_fatal_error("Metadata key 0x" + Twine::utohexstr(I->first)
                + " is too large (" + Twine(I->second.getSize())
                + ") for block size " + Twine(SVMTargetMachine::getBlockSize()));
    }
}

void MetadataCollectorPass::createLayout()
{
    /*
     * Assign an order to the keys (in "Layout"), and assign an absolute
     * address to each value. This is where we enforce alignment and
     * ensure that values do not cross flash block boundaries.
     *
     * This is a greedy algorithm that always chooses the next key as the one
     * which (1) wastes the least space, and (2) is largest.
     */

    uint32_t address = SVMELF::HdrSize + Dict.size() * 4;
    assert((address % 4) == 0);

    Layout_t pending;
    for (Dict_t::iterator I = Dict.begin(), E = Dict.end(); I != E; ++I)
        pending.push_back(I->first);

    while (!pending.empty()) {
        Layout_t::iterator bestI;
        unsigned bestWastage = (unsigned)-1;
        unsigned bestSize = 0;
        
        for (Layout_t::iterator I = pending.begin(), E = pending.end(); I != E; ++I) {
            MetadataValues &MV = Dict[*I];
            unsigned size = MV.getSize();
            unsigned wastage = MV.findFirstAddress(address) - address;
            
            if (wastage < bestWastage || (wastage == bestWastage && size > bestSize)) {
                bestI = I;
                bestWastage = wastage;
                bestSize = size;
            }
        }

        // Assign its final address
        MetadataValues &MV = Dict[*bestI];
        address = MV.findFirstAddress(address);
        MV.setAddress(address);
        address += MV.getSize();

        // Save this key
        Layout.push_back(*bestI);
        pending.erase(bestI);
    }
}

void MetadataCollectorPass::pack(Module &M)
{
    /*
     * Assemble all keys, values, and padding into a packed structure,
     * and store that in the .metadata section.
     */

    SmallVector<Constant*, 16> Members;
    LLVMContext &Ctx = M.getContext();
    Type *i16 = Type::getInt16Ty(Ctx);
    Type *i8 = Type::getInt8Ty(Ctx);
    Constant *pad = ConstantInt::get(i8, SVMTargetMachine::getPaddingByte());

    /*
     * First, add all metadata keys, as uint16_t values.
     */

    for (Layout_t::iterator I = Layout.begin(), E = Layout.end(); I != E; ++I) {
        MetadataValues &MV = Dict[*I];
        uint16_t stride;

        Layout_t::iterator NextI = I;
        ++NextI;
        if (NextI == E)
            stride = MV.getSize() | 0x8000;
        else
            stride = Dict[*NextI].getAddress() - MV.getAddress();

        Members.push_back(ConstantInt::get(i16, stride));
        Members.push_back(ConstantInt::get(i16, *I));
    }

    /*
     * Now alternate values and padding
     */

    for (Layout_t::iterator I = Layout.begin(), E = Layout.end(); I != E; ++I) {
        MetadataValues &MV = Dict[*I];
        uint16_t padding;

        Layout_t::iterator NextI = I;
        ++NextI;
        if (NextI == E)
            padding = 0;
        else
            padding = Dict[*NextI].getAddress() - MV.getAddress() - MV.getSize();

        Members.push_back(MV.getStruct());
        
        while (padding) {
            padding--;
            Members.push_back(pad);
        }
    }

    /*
     * Create the final packed struct!
     */

    Constant *Struct = ConstantStruct::getAnon(Members, true);
    GlobalVariable *GV = new GlobalVariable(M, Struct->getType(),
        true, GlobalValue::ExternalLinkage, Struct, "", 0, false);

    GV->setAlignment(1);
    GV->setName(SVMDecorations::META);
    GV->setSection(".metadata");
}

ModulePass *llvm::createMetadataCollectorPass()
{
    return new MetadataCollectorPass();
}
