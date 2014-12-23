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

#include "SVMMemoryLayout.h"
#include "SVMTargetMachine.h"
#include "SVMSymbolDecoration.h"
#include "SVMFixupKinds.h"
#include "SVMMCAsmBackend.h"

using namespace llvm;


void SVMMemoryLayout::AllocateSections(MCAssembler &Asm, const MCAsmLayout &Layout)
{
    memset(spsMemSize, 0, sizeof spsMemSize);
    memset(spsDiskSize, 0, sizeof spsDiskSize);
    bssAlign = 1;

    // Leave one free block at the beginning of DEBUG, for a special message.
    spsMemSize[SPS_DEBUG] = SVMTargetMachine::getBlockSize();
    spsDiskSize[SPS_DEBUG] = spsMemSize[SPS_DEBUG];

    for (MCAssembler::const_iterator it = Asm.begin(), ie = Asm.end();
        it != ie; ++it) {
        const MCSectionData *SD = &*it;
        SVMProgramSection sps = getSectionKind(SD);

        switch (sps) {

        case SPS_BSS:
            // Also track BSS segment alignment, then fall through...
            bssAlign = std::max(bssAlign, SD->getAlignment());

        case SPS_RO:
        case SPS_RW_Z:
        case SPS_RW_PLAIN:
        case SPS_DEBUG:
        case SPS_META:
            spsMemSize[sps] = RoundUpToAlignment(spsMemSize[sps], SD->getAlignment());
            spsDiskSize[sps] = RoundUpToAlignment(spsDiskSize[sps], SD->getAlignment());

            SectionOffsetMap[SD] = spsDiskSize[sps];

            spsMemSize[sps] += getSectionMemSize(SD, Layout);
            spsDiskSize[sps] += Layout.getSectionAddressSize(SD);
            break;

        default:
            break;
        }
    }

    unsigned memUsed = getSectionMemAddress(SPS_END) - SVMTargetMachine::getRAMBase();
    unsigned memMax = SVMTargetMachine::getRAMSize();
    if (memUsed > memMax)
        report_fatal_error("Application is too large to fit in RAM! Need "
            + Twine(memUsed) + " bytes, which exceeds the maximum of " + Twine(memMax));
}

uint32_t SVMMemoryLayout::getSectionDiskSize(enum SVMProgramSection s) const
{
    switch (s) {
    case SPS_META:
    case SPS_DEBUG:
    case SPS_RW_PLAIN:
    case SPS_RO:
    case SPS_RW_Z:      return spsDiskSize[s];
    case SPS_BSS:       return 0;
    default:            llvm_unreachable("Bad SVM Program Section");
    }
}

uint32_t SVMMemoryLayout::getSectionMemSize(enum SVMProgramSection s) const
{
    switch (s) {
    case SPS_RO:
    case SPS_RW_Z:
    case SPS_RW_PLAIN:
    case SPS_BSS:       return spsMemSize[s];
    case SPS_META:
    case SPS_DEBUG:     return 0;
    default:            llvm_unreachable("Bad SVM Program Section");
    }
}

uint32_t SVMMemoryLayout::getSectionDiskOffset(enum SVMProgramSection s) const
{
    uint32_t blockAlign = SVMTargetMachine::getBlockSize();

    /*
     * A note on disk alignment: RO must be aligned on disk, since we're fetching
     * from it directly, the the other sections need no extra alignment.
     *
     * (But we align DEBUG anyway, so we can have a page of padding between
     * non-debug and debug data)
     */

    switch (s) {
    case SPS_META:      return SVMELF::HdrSize;
    case SPS_RO:        return RoundUpToAlignment(getSectionDiskEnd(SPS_META), blockAlign);
    case SPS_RW_Z:      return getSectionDiskEnd(SPS_RO);
    case SPS_BSS:       return 0;
    case SPS_DEBUG:     return RoundUpToAlignment(getSectionDiskEnd(SPS_RW_Z), blockAlign);
    case SPS_RW_PLAIN:  return RoundUpToAlignment(getSectionDiskEnd(SPS_DEBUG), blockAlign);
    case SPS_END:       return getSectionDiskEnd(SPS_RW_PLAIN);
    default:            llvm_unreachable("Bad SVM Program Section");
    }
}

uint32_t SVMMemoryLayout::getSectionDiskEnd(enum SVMProgramSection s) const
{
    return getSectionDiskOffset(s) + spsDiskSize[s];
}

uint32_t SVMMemoryLayout::getSectionMemAddress(enum SVMProgramSection s) const
{
    uint32_t flashBase = SVMTargetMachine::getFlashBase();
    uint32_t ramBase = SVMTargetMachine::getRAMBase();

    switch (s) {
    case SPS_RO:        return flashBase;
    case SPS_RW_Z:      return ramBase;
    case SPS_RW_PLAIN:  return ramBase;
    case SPS_BSS:       return getSectionMemAddress(SPS_RW_PLAIN) +
                            RoundUpToAlignment(spsMemSize[SPS_RW_PLAIN], bssAlign);
    case SPS_END:       return getSectionMemAddress(SPS_BSS) + spsMemSize[SPS_BSS];
    case SPS_META:
    case SPS_DEBUG:     return 0;
    default:            llvm_unreachable("Bad SVM Program Section");
    }
}

void SVMMemoryLayout::RecordRelocation(const MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCFragment *Fragment,
    const MCFixup &Fixup, MCValue Target, uint64_t &FixedValue)
{
    SVM::Fixups kind = (SVM::Fixups) Fixup.getKind();
    switch (kind) {

        case SVM::fixup_fnstack: {
            /*
             * Function stack adjustment annotation. The adjustment amount is
             * stored in the first argument of a binary op.
             * (See SVMMCCodeEmitter::EncodeInstruction)
             *
             * We store this offset for later, in our FNStackMap, indexed
             * by the address of the FNStack pseudo-op itself.
             */
            FNStackMap[std::make_pair(Fragment->getParent(),
                Layout.getFragmentOffset(Fragment))] = (int)Target.getConstant();
            break;
        }
    
        default: {
            /*
             * All other fixups are applied later, in ApplyLateFixups().
             * This is necessary because other fixup types can depend on
             * the FNStack values collected above.
             */
            SVMLateFixup LF((MCFragment *) Fragment, Fixup, Target);
            LateFixupList.push_back(LF);

            // This gets OR'ed with the fixup later. Zero it.
            FixedValue = 0;
            break;
        }
    }
}

void SVMMemoryLayout::ApplyLateFixups(const MCAssembler &Asm,
    const MCAsmLayout &Layout)
{
    for (LateFixupList_t::iterator i = LateFixupList.begin();
        i != LateFixupList.end(); ++i) {
        SVMLateFixup &F = *i;
        MCDataFragment *DF = dyn_cast<MCDataFragment>(F.Fragment);
        assert(DF);
        MCSectionData *SD = DF->getParent();

        /*
         * Normally we use specially formatted "code addresses" for
         * labels in the text segment. These aren't real VAs, they are
         * packed words that include a 24-bit address and a stack adjustment.
         *
         * These addresses need to be formed for pointers that are stored
         * either in the text or the data segments, since they need to take
         * effect for function pointers.
         *
         * In debug sections, however, they're quite unhelpful- debuggers
         * expect real VAs. So we'll explicitly disable this feature when
         * performing a fixup that is located in a debug section.
         */
        bool useCodeAddresses = getSectionKind(SD) != SPS_DEBUG;

        SVMAsmBackend::ApplyStaticFixup(F.Kind,
            &DF->getContents().data()[F.Offset],
            getSymbol(Asm, Layout, F.Target, useCodeAddresses).Value);
    }
}

uint32_t SVMMemoryLayout::getEntryAddress(const MCAssembler &Asm,
    const MCAsmLayout &Layout) const
{
    MCSymbol *S = Asm.getContext().LookupSymbol("main");
    if (!S || !S->isDefined())
        report_fatal_error("No entry point exists. Is main() defined?");

    SVMSymbolInfo SI = getSymbol(Asm, Layout, S);
    assert(SI.Kind == SVMSymbolInfo::LOCAL);
    return SI.Value;
}

SVMSymbolInfo SVMMemoryLayout::getSymbol(const MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCSymbol *S, bool useCodeAddresses) const
{
    SVMSymbolInfo SI;
    SVMDecorations Deco;
    StringRef Name = Deco.Decode(S->getName());
    const MCSymbol *AS = &S->AliasedSymbol();
    const MCSymbolData *SD = &Asm.getSymbolData(*AS);

    if (Deco.isSys) {
        // Numeric syscall

        if (Deco.sysNumber > 0x3FFF)
            report_fatal_error("Syscall number " + Twine(Deco.sysNumber) +
                " is out of range.");

        SI.Kind = SVMSymbolInfo::SYS;
        SI.Value = 0x80000000 | (Deco.sysNumber << 16) | Deco.isTailCall;
        return SI;
    }

    if (!AS->isDefined())
        report_fatal_error("Taking address of undefined symbol '" +
            Twine(Name) + "'");

    // Symbol has a value in our module. Calculate the full virtual address.
    uint32_t Offset = Layout.getSymbolOffset(SD);
    const MCSectionData *SecD = &Asm.getSectionData(AS->getSection());
    uint32_t VA = getSectionMemAddress(SecD) + Offset + Deco.offset;

    if (useCodeAddresses && AS->getSection().getKind().isText()) {
        /*
         * Code address:
         *   - Relative to segment base
         *   - Must be 32-bit aligned
         *   - Includes optional SP adjustment from FNSTACK pseudo-ops
         *   - Includes optional call / tail-call decoration
         */
         
        assert(Deco.offset == 0);
        uint32_t shortVA = VA & 0xfffffc;

        /*
         * We can reach this error if some code wasn't aligned properly,
         * or if someone is calling this function with useCodeAddresses==true
         * when they shouldn't be!
         */
        if ((VA & 0xfffffffc) != VA)
            report_fatal_error("Code symbol '" + Twine(Name) +
                "' has illegal address 0x" + Twine::utohexstr(VA));

        FNStackMap_t::const_iterator I = FNStackMap.find(std::make_pair(SecD, Offset));
        int SPAdj = I == FNStackMap.end() ? 0 : I->second;
        assert(SPAdj >= 0 && !(SPAdj & 3) && SPAdj <= (0x7F * 4));
        SPAdj <<= 22;

        if (Deco.isCall) {
            // A Call, with SP adjustment and tail-call flag
            SI.Value = shortVA | SPAdj | Deco.isTailCall;
            SI.Kind = SVMSymbolInfo::CALL;

        } else if (Deco.isLongBranch) {
            // Encode the Long Branch addrop
            SI.Value = 0xE0000000 | shortVA;
            SI.Kind = SVMSymbolInfo::LB;

        } else {
            // Normal undecorated address. Still includes an SP adjustment
            // if one is present for this symbol's address, though.
            // This is important for function pointers, including the entry
            // point address for main().
            //
            // We also set the LSB, in order to differentiate a valid function
            // address from a possible NULL. We could set either of the two LSBs
            // or the MSB, but this approach makes function addresses clearly
            // distinct from normal VAs even with SPAdj==0.

            SI.Value = shortVA | SPAdj | 1;
            SI.Kind = SVMSymbolInfo::LOCAL;
        }

    } else {
        /*
         * Data address. No decoration at all.
         */

        SI.Value = VA;
        SI.Kind = SVMSymbolInfo::LOCAL;
    }

    return SI;
}

SVMSymbolInfo SVMMemoryLayout::getSymbol(const MCAssembler &Asm,
    const MCAsmLayout &Layout, MCValue Value, bool useCodeAddresses) const
{
    /*
     * Convert an MCValue into a resolved SVMSymbolInfo,
     * by evaluating (SymA - SymB + Constant).
     */

    int64_t CValue = Value.getConstant();
    const MCSymbolRefExpr *SA = Value.getSymA();
    const MCSymbolRefExpr *SB = Value.getSymB();
    SVMSymbolInfo SIA, SIB;
    
    if (SA) SIA = getSymbol(Asm, Layout, &SA->getSymbol(), useCodeAddresses);
    if (SB) SIB = getSymbol(Asm, Layout, &SB->getSymbol(), useCodeAddresses);

    if ((SIA.Kind == SVMSymbolInfo::NONE || SIA.Kind == SVMSymbolInfo::LOCAL) &&
        (SIB.Kind == SVMSymbolInfo::NONE || SIB.Kind == SVMSymbolInfo::LOCAL)) {

        // Arithmetic is allowed.
        SIA.Kind = SVMSymbolInfo::LOCAL;
        SIA.Value = CValue + SIA.Value - SIB.Value;
        return SIA;
    }

    if (SIA.Kind == SIB.Kind && SIA.Value == SIB.Value) {
        // Two special symbols cancel out, yield a normal constant
        SIA.Kind = SVMSymbolInfo::LOCAL;
        SIA.Value = CValue;
        return SIA;
    }

    if (SIA.Kind != SVMSymbolInfo::NONE && SIB.Kind != SVMSymbolInfo::NONE)
        report_fatal_error("Can't take the difference of special symbols '"
            + Twine(SA->getSymbol().getName()) + "' and '"
            + Twine(SB->getSymbol().getName()) + "'");

    if (SIB.Kind != SVMSymbolInfo::NONE)
        report_fatal_error("Can't negate special symbol '"
            + Twine(SB->getSymbol().getName()) + "'");

    if (CValue != 0)
        report_fatal_error("Can't add constant to special symbol '"
            + Twine(SA->getSymbol().getName()) + "'");

    return SIA;
}

SVMProgramSection SVMMemoryLayout::getSectionKind(const MCSectionData *SD) const
{
    /*
     * Categorize a MCSection as one of the several predefined
     * SVMProgramSection types. This determines which program section
     * we'll lump it in with, or whether it'll be included as debug-only.
     *
     * This value can be specifically overridden with setSectionKind().
     * If we have no override, we calculate the proper section kind
     * using the section's name, size, type, and other data.
     */

    SectionKindOverrides_t::const_iterator it = SectionKindOverrides.find(SD);
    if (it != SectionKindOverrides.end())
        return it->second;

    const MCSection *S = &SD->getSection();
    SectionKind k = S->getKind();
    const MCSectionELF *SE = dyn_cast<MCSectionELF>(S);
    assert(SE);
    StringRef Name = SE->getSectionName();

    if (Name == ".metadata")
        return SPS_META;

    if (Name.startswith(".debug_"))
        return SPS_DEBUG;

    if (k.isBSS())
        return SPS_BSS;

    if (SE->getType() == ELF::SHT_PROGBITS) {
        if (k.isReadOnly() || k.isText())
            return SPS_RO;
        if (k.isGlobalWriteableData())
            return SPS_RW_PLAIN;
    }

    return SPS_DEBUG;
}

void SVMMemoryLayout::setSectionKind(const MCSectionData *SD, SVMProgramSection kind)
{
    SectionKindOverrides[SD] = kind;
}

uint32_t SVMMemoryLayout::getSectionMemSize(const MCSectionData *SD, const MCAsmLayout &Layout) const
{
    SectionMemSizeOverrides_t::const_iterator it = SectionMemSizeOverrides.find(SD);
    if (it == SectionMemSizeOverrides.end())
        return Layout.getSectionAddressSize(SD);
    else
        return it->second;
}

void SVMMemoryLayout::setSectionMemSize(const MCSectionData *SD, uint32_t size)
{
    SectionMemSizeOverrides[SD] = size;
}

uint32_t SVMMemoryLayout::getSectionDiskOffset(const MCSectionData *SD) const
{
    SectionOffsetMap_t::const_iterator it = SectionOffsetMap.find(SD);

    if (it == SectionOffsetMap.end())
        llvm_unreachable("Section not found in offset map");

    return it->second + getSectionDiskOffset(getSectionKind(SD));
}

uint32_t SVMMemoryLayout::getSectionMemAddress(const MCSectionData *SD) const
{
    switch (getSectionKind(SD)) {

    case SPS_META:
    case SPS_DEBUG:
        return 0;
    
    default: {
        SectionOffsetMap_t::const_iterator it = SectionOffsetMap.find(SD);

        if (it == SectionOffsetMap.end())
            llvm_unreachable("Section not found in offset map");

        return it->second + getSectionMemAddress(getSectionKind(SD));    
    }
    }
}
