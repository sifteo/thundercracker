/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMELFProgramWriter.h"
#include "SVMSymbolDecoration.h"
#include "SVMMCAsmBackend.h"
using namespace llvm;


SVMELFProgramWriter::SVMELFProgramWriter(raw_ostream &OS)
    : MCObjectWriter(OS, true)
{
    ramTopAddr = SVMTargetMachine::getRAMBase();
    flashTopAddr = SVMTargetMachine::getFlashBase();    
}

void SVMELFProgramWriter::writePadding(unsigned N)
{
    /*
     * Instead of padding with zeroes, pad with ones.
     * These programs get stored in flash memory, and 0xFF
     * is what the flash erases to.
     */

    const char Ones[16] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    };

    for (unsigned i = 0, e = N / 16; i != e; ++i)
        OS << StringRef(Ones, 16);

    OS << StringRef(Ones, N % 16);
}

void SVMELFProgramWriter::writeELFHeader(const MCAssembler &Asm,
    const MCAsmLayout &Layout, unsigned PHNum, uint64_t &HdrSize)
{
    unsigned SHNum = PHNum + 1;
    unsigned PHSize = PHNum * sizeof(ELF::Elf32_Phdr);
    unsigned SHSize = SHNum * sizeof(ELF::Elf32_Shdr);
    HdrSize = sizeof(ELF::Elf32_Ehdr) + PHSize + SHSize;
    
    unsigned PHOff = sizeof(ELF::Elf32_Ehdr);
    unsigned SHOff = PHOff + PHSize;
    
    Write8(0x7f);                                   // e_ident
    Write8('E');
    Write8('L');
    Write8('F');
    Write8(ELF::ELFCLASS32);
    Write8(ELF::ELFDATA2LSB);
    Write8(ELF::EV_CURRENT);
    Write8(ELF::ELFOSABI_NONE);
    Write8(0);                                      // ABI Version
    WriteZeros(ELF::EI_NIDENT - ELF::EI_PAD);
    
    Write16(ELF::ET_EXEC);                          // e_type
    Write16(ELF::EM_ARM);                           // e_machine
    Write32(ELF::EV_CURRENT);                       // e_version
    Write32(getEntryAddress(Asm, Layout));          // e_entry
    Write32(PHOff);                                 // e_phoff
    Write32(SHOff);                                 // e_shoff
    Write32(0);                                     // e_flags
    Write16(sizeof(ELF::Elf32_Ehdr));               // e_ehsize
    Write16(sizeof(ELF::Elf32_Phdr));               // e_phentsize
    Write16(PHNum);                                 // e_phnum
    Write16(sizeof(ELF::Elf32_Shdr));               // e_shentsize
    Write16(SHNum);                                 // e_shnum
    Write16(PHNum);                                 // e_shstrndx
}

void SVMELFProgramWriter::writeProgramHeader(const MCAsmLayout &Layout,
    const MCSectionData &SD, uint64_t &FileOff)
{
    /*
     * Program headers are used by the runtime's loader. This is the
     * canonical data used to set up the binary's address space before
     * executing it.
     */
    
    uint64_t AddressSize = Layout.getSectionAddressSize(&SD);
    uint64_t FileSize = Layout.getSectionFileSize(&SD);
    unsigned Alignment = SD.getAlignment();
    unsigned VirtualAddr = SectionBaseAddrMap[&SD];
    SectionKind Kind = SD.getSection().getKind();
    uint32_t Flags = ELF::PF_R;

    if (Kind.isText())                  Flags |= ELF::PF_X;
    if (Kind.isGlobalWriteableData())   Flags |= ELF::PF_W;

    FileOff = RoundUpToAlignment(FileOff, Alignment);

    Write32(ELF::PT_LOAD);                          // p_type
    Write32(FileOff);                               // p_offset
    Write32(VirtualAddr);                           // p_vaddr
    Write32(0);                                     // p_paddr
    Write32(FileSize);                              // p_filesz
    Write32(AddressSize);                           // p_memsz
    Write32(Flags);                                 // p_flags
    Write32(Alignment);                             // p_align

    FileOff += FileSize;
}

void SVMELFProgramWriter::writeStrtabSectionHeader()
{
    /*
     * We don't really care about section names yet, but this is just
     * a stub so that tools like objdump will work.
     */

     Write32(0);                                     // sh_name
     Write32(ELF::SHT_STRTAB);                       // sh_type
     Write32(0);                                     // sh_flags
     Write32(0);                                     // sh_addr
     Write32(8);                                     // sh_offset
     Write32(1);                                     // sh_size
     Write32(0);                                     // sh_link
     Write32(0);                                     // sh_info
     Write32(1);                                     // sh_addralign
     Write32(0);                                     // sh_entsize
}

void SVMELFProgramWriter::writeProgSectionHeader(const MCAsmLayout &Layout,
    const MCSectionData &SD, uint64_t &FileOff)
{
    /*
     * Section headers are used only by debug tools, not by the runtime.
     * For convenience, we create a set of section headers corresponding
     * to each program header.
     */
    
    uint64_t FileSize = Layout.getSectionFileSize(&SD);
    unsigned Alignment = SD.getAlignment();
    unsigned VirtualAddr = SectionBaseAddrMap[&SD];
    SectionKind Kind = SD.getSection().getKind();
    uint32_t Flags = ELF::SHF_ALLOC;

    FileOff = RoundUpToAlignment(FileOff, Alignment);

    if (Kind.isText())                  Flags |= ELF::SHF_EXECINSTR;
    if (Kind.isGlobalWriteableData())   Flags |= ELF::SHF_WRITE;

    Write32(0);                                     // sh_name
    Write32(ELF::SHT_PROGBITS);                     // sh_type
    Write32(Flags);                                 // sh_flags
    Write32(VirtualAddr);                           // sh_addr
    Write32(FileOff);                               // sh_offset
    Write32(FileSize);                              // sh_size
    Write32(0);                                     // sh_link
    Write32(0);                                     // sh_info
    Write32(Alignment);                             // sh_addralign
    Write32(0);                                     // sh_entsize

    FileOff += FileSize;
}
void SVMELFProgramWriter::writeProgramData(MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCSectionData &SD, uint64_t &FileOff)
{
    unsigned Alignment = SD.getAlignment();
    unsigned Padding = OffsetToAlignment(FileOff, Alignment);
    
    writePadding(Padding);
    FileOff += Padding;
    FileOff += Layout.getSectionFileSize(&SD);

    Asm.WriteSectionData(&SD, Layout);
}   

void SVMELFProgramWriter::collectProgramSections(const MCAssembler &Asm,
    const MCAsmLayout &Layout, SectionDataList &Sections)
{
    /*
     * Put all sections in order, and assign addresses as needed.
     */
    
    // All RAM data
    for (MCAssembler::const_iterator it = Asm.begin(), ie = Asm.end();
        it != ie; ++it) {
        SectionKind k = it->getSection().getKind();
        if (k.isGlobalWriteableData() && !k.isBSS()) {
            const MCSectionData *SD = &*it;
            Sections.push_back(SD);

            if (!SectionBaseAddrMap.count(SD)) {
                SectionBaseAddrMap[SD] = ramTopAddr;
                ramTopAddr += Layout.getSectionAddressSize(SD);
            }
        }
    }

    // All flash data
    for (MCAssembler::const_iterator it = Asm.begin(), ie = Asm.end();
        it != ie; ++it) {
        SectionKind k = it->getSection().getKind();
        if (k.isReadOnly() || k.isText()) {
            const MCSectionData *SD = &*it;
            Sections.push_back(SD);

            if (!SectionBaseAddrMap.count(SD)) {
                SectionBaseAddrMap[SD] = flashTopAddr;
                flashTopAddr += Layout.getSectionAddressSize(SD);
            }
        }
    }
}

void SVMELFProgramWriter::WriteObject(MCAssembler &Asm, const MCAsmLayout &Layout)
{
    SectionDataList ProgSections;
    collectProgramSections(Asm, Layout, ProgSections);
    applyLateFixups(Asm, Layout);
    
    uint64_t FileOff, HdrSize;
    writeELFHeader(Asm, Layout, ProgSections.size(), HdrSize);
    
    FileOff = HdrSize;
    for (SectionDataList::iterator it = ProgSections.begin(), ie = ProgSections.end();
        it != ie; ++it)
        writeProgramHeader(Layout, **it, FileOff);

    FileOff = HdrSize;
    for (SectionDataList::iterator it = ProgSections.begin(), ie = ProgSections.end();
        it != ie; ++it)
        writeProgSectionHeader(Layout, **it, FileOff);

    writeStrtabSectionHeader();

    FileOff = HdrSize;
    for (SectionDataList::iterator it = ProgSections.begin(), ie = ProgSections.end();
        it != ie; ++it)
        writeProgramData(Asm, Layout, **it, FileOff);
}

void SVMELFProgramWriter::RecordRelocation(const MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCFragment *Fragment,
    const MCFixup &Fixup, MCValue Target, uint64_t &FixedValue)
{
    SVM::Fixups kind = (SVM::Fixups) Fixup.getKind();
        
    switch (kind) {
        
    case SVM::fixup_fnstack:
        /*
         * Function stack adjustment annotation. The adjustment amount is
         * stored in the first argument of a binary op.
         * (See SVMMCCodeEmitter::EncodeInstruction)
         *
         * We store this offset for later, in our FNStackMap, indexed
         * by the address of the FNStack pseudo-op itself.
         */
        FNStackMap[Layout.getFragmentOffset(Fragment)] =
            (int) Target.getConstant();
        break;
    
    default:
        /*
         * All other fixups are applied later, in ApplyLateFixups().
         * This is necessary because other fixup types can depend on
         * the FNStack values collected above.
         */
        SVMLateFixup LF((MCFragment *) Fragment, Fixup, Target);
        LateFixupList.push_back(LF);
        break;
    }
}

void SVMELFProgramWriter::applyLateFixups(const MCAssembler &Asm,
    const MCAsmLayout &Layout)
{
    for (LateFixupList_t::iterator i = LateFixupList.begin();
        i != LateFixupList.end(); ++i) {
        SVMLateFixup &F = *i;

        SVMSymbolInfo SI = getSymbol(Asm, Layout, F.Target);
        uint32_t Value = SI.Value;

        const MCFixupKindInfo &KI = SVMAsmBackend::getStaticFixupKindInfo(F.Kind);
        int Bits = KI.TargetSize;
        unsigned Offset = F.Offset;

        switch (F.Kind) {

        case SVM::fixup_bcc:
        case SVM::fixup_b:
            // PC-relative halfword count
            Value = (Value - 4) / 2;
            break;

        case SVM::fixup_relcpi:
            // PC-relative word count
            Value = (Value - 4) / 4;
            break;

        case SVM::fixup_abscpi:
            // Word count from beginning of block
            Value = (Value / 4) & 0x7F;
            break;

        default:
            break;
        }

        MCDataFragment *DF = dyn_cast<MCDataFragment>(F.Fragment);
        assert(DF);

        assert(Bits > 0);
        uint64_t BitMask = ((uint64_t)1 << Bits) - 1;
        assert((Value & ~BitMask) == 0 || (Value | BitMask) == (uint64_t)-1);
        Value &= BitMask;

        do {
            DF->getContents().data()[Offset] |= Value;
            Bits -= 8;
            Offset++;
            Value >>= 8;
        } while (Bits > 0);
    }
}

uint32_t SVMELFProgramWriter::getEntryAddress(const MCAssembler &Asm,
    const MCAsmLayout &Layout)
{
    MCSymbol *S = SVMEntryPoint::findEntry(Asm.getContext());
    if (!S)
        report_fatal_error("No entry point exists. Is \"" +
            Twine(SVMEntryPoint::getPreferredSignature()) + "\" defined?");

    SVMSymbolInfo SI = getSymbol(Asm, Layout, S);
    assert(SI.Kind == SVMSymbolInfo::LOCAL);
    return SI.Value;
}

SVMSymbolInfo SVMELFProgramWriter::getSymbol(const MCAssembler &Asm,
    const MCAsmLayout &Layout, MCValue Value)
{
    /*
     * Convert an MCValue into a resolved SVMSymbolInfo,
     * by evaluating (SymA - SymB + Constant).
     */

    int64_t CValue = Value.getConstant();
    const MCSymbolRefExpr *SA = Value.getSymA();
    const MCSymbolRefExpr *SB = Value.getSymB();
    SVMSymbolInfo SIA, SIB;
    
    if (SA) SIA = getSymbol(Asm, Layout, &SA->getSymbol());
    if (SB) SIB = getSymbol(Asm, Layout, &SB->getSymbol());

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

SVMSymbolInfo SVMELFProgramWriter::getSymbol(const MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCSymbol *S)
{
    const MCSymbolData *SD = &Asm.getSymbolData(*S);
    SVMSymbolInfo SI;
    SVMDecorations Deco;
    StringRef Name = Deco.Decode(S->getName());

    if (Deco.isSys) {
        // Numeric syscall
        SI.Kind = SVMSymbolInfo::SYS;
        SI.Value = 0x80000000
            | ((Deco.sysNumber & 0x7FFF) << 1)
            | Deco.isTailCall;
        return SI;
    }

    if (!S->isDefined())
        report_fatal_error("Taking address of undefined symbol '" +
            Twine(Name) + "'");
    
    // Symbol has a value in our module
    uint32_t Offset = Layout.getSymbolOffset(SD);

    if (S->getSection().getKind().isText()) {
        /*
         * Code address:
         *   - Relative to segment base
         *   - Must be 32-bit aligned
         *   - Includes optional SP adjustment from FNSTACK pseudo-ops
         *   - Includes optional call / tail-call decoration
         */

        if ((Offset & 0xfffffc) != Offset)
            report_fatal_error("Code symbol '" + Twine(Name) +
                "' has illegal address " + Twine::utohexstr(Offset));
    
        int SPAdj = FNStackMap[Offset];
        if (SPAdj < 0 || (SPAdj & 3) || SPAdj >= 512)
            report_fatal_error("Code symbol '" + Twine(Name) +
                "' has unsupported stack size of " + Twine(Offset) + " bytes");
        
        SI.Value = Offset | (SPAdj << 22) | Deco.isTailCall;
        SI.Kind = Deco.isCall ? SVMSymbolInfo::CALL : SVMSymbolInfo::LOCAL;

    } else {
        /*
         * Data address
         */

        SI.Value = getSectionBaseForSymbol(Asm, Layout, S) + Offset;
        SI.Kind = SVMSymbolInfo::LOCAL;
    }
        
    return SI;
}

uint32_t SVMELFProgramWriter::getSectionBaseForSymbol(const MCAssembler &Asm,
    const MCAsmLayout &Layout, const MCSymbol *S)
{    
    // Is there already a memoized address for this section?
    const MCSectionData *Section = &Asm.getSectionData(S->getSection());
    SectionBaseAddrMap_t::iterator it = SectionBaseAddrMap.find(Section);

    if (it == SectionBaseAddrMap.end()) {
        // Generate SectionAddrs for all sections in the layout
        SectionDataList ProgSections;
        collectProgramSections(Asm, Layout, ProgSections);
        it = SectionBaseAddrMap.find(Section);
    }

    if (it == SectionBaseAddrMap.end())
        report_fatal_error("Cannot determine section for symbol '" +
            Twine(S->getName()) + "'");
    
    return it->second;
}

MCObjectWriter *llvm::createSVMELFProgramWriter(raw_ostream &OS)
{
    return new SVMELFProgramWriter(OS);
}
