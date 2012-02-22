#include "svmruntime.h"
#include "elfdefs.h"
#include "flashlayer.h"
#include "svm.h"
#include "svmvalidator.h"

#include <string.h>

using namespace Svm;

SvmRuntime::SvmRuntime()
{
}

void SvmRuntime::run(uint16_t appId)
{
    currentAppPhysAddr = 0;  // TODO: look this up via appId
    if (!loadElfFile(currentAppPhysAddr, 12345))
        return;

    LOG(("loaded. entry: 0x%x text start: 0x%x\n", progInfo.entry, progInfo.textRodata.start));

    unsigned entryPoint = progInfo.textRodata.start + progInfo.entry;
    if (!FlashLayer::getRegion(entryPoint, FlashLayer::BLOCK_SIZE, &flashRegion)) {
        LOG(("failed to get entry block\n"));
        return;
    }

    cpu.init(this);

    // TODO: This actually needs to be like a call SVC, not just a branch
    cpu.setReg(SvmCpu::REG_PC, reinterpret_cast<reg_t>(flashRegion.data())
        + (progInfo.textRodata.vaddr & 0xFFFFFF));

    SvmValidator::validBytes(flashRegion.data(), flashRegion.size());
    cpu.run();
}

/*
SVC encodings:

  11011111 0xxxxxxx     (1) Indirect operation
  11011111 10xxxxxx     (2) Direct syscall #0-63
  11011111 110xxxxx     (3) SP = validate(SP - imm5*4)
  11011111 11100rrr     (4) r8-9 = validate(rN)
  11011111 11101rrr     (5) Reserved
  11011111 11110rrr     (6) Call validate(rN), with SP adjust
  11011111 11111rrr     (7) Tail call validate(rN), with SP adjust
*/
void SvmRuntime::svc(uint8_t imm8)
{
    LOG(("svc, imm8 %d\n", imm8));
    if ((imm8 & (1 << 7)) == 0) {
        svcIndirectOperation(imm8);
        return;
    }
    if ((imm8 & (0x3 << 6)) == (0x2 << 6)) {
        uint8_t syscallNum = imm8 & 0x3f;
        LOG(("direct syscall #%d\n", syscallNum));
        reg_t result = SyscallTable[syscallNum](cpu.reg(0), cpu.reg(1), cpu.reg(2), cpu.reg(3),
                                                cpu.reg(4), cpu.reg(5), cpu.reg(6), cpu.reg(7));
        cpu.setReg(0, result);
        return;
    }
    if ((imm8 & (0x7 << 5)) == (0x6 << 5)) {
        LOG(("SP = validate(SP - imm5*4)\n"));
        uint8_t imm5 = imm8 & 0x1f;
        cpu.setReg(SvmCpu::REG_SP, validate(cpu.reg(SvmCpu::REG_SP) - (imm5 * 4)));
        return;
    }

    uint8_t sub = (imm8 >> 3) & 0x1f;
    unsigned r = imm8 & 0x7;
    switch (sub) {
    case 0x1c:  { // 0b11100
        LOG(("svc: r8-9 = validate(r%d)\n", r));
        uint32_t addr = validate(cpu.reg(r));
        setBasePtrs(addr);
        return;
    }
    case 0x1d:  // 0b11101
        ASSERT(0 && "svc: reserved\n");
        return;
    case 0x1e: { // 0b11110
        LOG(("svc: Call validate(rN), with SP adjust\n"));
        return;
    }
    case 0x1f:  // 0b11110
        LOG(("svc: Tail call validate(rN), with SP adjust\n"));
        return;
    default:
        ASSERT(0 && "unknown sub op for svc");
        break;
    }
    ASSERT(0 && "unhandled SVC");
}

/*
When the MSB of the SVC immediate is clear, the other 7 bits are multiplied by
four and added to the base of the current page, to form the address of a
32-bit literal. This 32-bit literal encodes an indirect operation to perform:

  0nnnnnnn aaaaaaaa aaaaaaaa aaaaaa00   (1) Call validate(F+a*4), SP -= n*4
  0nnnnnnn aaaaaaaa aaaaaaaa aaaaaa01   (2) Tail call validate(F+a*4), SP -= n*4
  0xxxxxxx xxxxxxxx xxxxxxxx xxxxxx1x   (3) Reserved
  10nnnnnn nnnnnnnn iiiiiiii iiiiiii0   (4) Syscall #0-8191, with #imm15
  10nnnnnn nnnnnnnn iiiiiiii iiiiiii1   (4) Tail syscall #0-8191, with #imm15
  110nnnnn aaaaaaaa aaaaaaaa aaaaaaaa   (5) Addrop #0-31 on validate(a)
  111nnnnn aaaaaaaa aaaaaaaa aaaaaaaa   (6) Addrop #0-31 on validate(F+a)

  (F = 0x80000000, flash segment virtual address)
*/
void SvmRuntime::svcIndirectOperation(uint8_t imm8)
{
    // we already know the MSB of imm8 is clear by the fact that we're being called here.

    reg_t instructionBase = cpu.reg(SvmCpu::REG_PC) - 2;
    uintptr_t blockMask = ~(uintptr_t)(FlashLayer::BLOCK_SIZE - 1);
    uint32_t *blockBase = reinterpret_cast<uint32_t*>(instructionBase & blockMask);
    uint32_t literal = blockBase[imm8];

    LOG(("indirect, literal 0x%x @ 0x%x\n", literal, cache2virtFlash(reinterpret_cast<reg_t>(blockBase + imm8))));

    if ((literal & CallMask) == CallTest) {
        LOG(("indirect call\n"));
        unsigned a = (literal >> 2) & 0x3fffff;
        unsigned n = (literal >> 24) & 0x7f;

        reg_t addr = validate(VIRTUAL_FLASH_BASE + (a * 4));
        // TODO: call it
    }
    else if ((literal & TailCallMask) == TailCallTest) {
        LOG(("indirect tail call\n"));
    }
    else if ((literal & IndirectSyscallMask) == IndirectSyscallTest) {
        unsigned imm15 = (literal >> 16) & 0x3ff;
        LOG(("indirect syscall #%d\n", imm15));
        reg_t result = SyscallTable[imm15](cpu.reg(0), cpu.reg(1), cpu.reg(2), cpu.reg(3),
                                           cpu.reg(4), cpu.reg(5), cpu.reg(6), cpu.reg(7));
        cpu.setReg(0, result);
    }
    else if ((literal & TailSyscallMask) == TailSyscallTest) {
        LOG(("indirect tail syscall\n"));
    }
    else if ((literal & AddropMask) == AddropTest) {
        unsigned opnum = (literal >> 24) & 0x1f;
        reg_t a = validate(literal & 0xffffff);
        addrOp(opnum, a);
    }
    else if ((literal & AddropFlashMask) == AddropFlashTest) {
        unsigned opnum = (literal >> 24) & 0x1f;
        reg_t a = validate(VIRTUAL_FLASH_BASE + (literal & 0xffffff));
        addrOp(opnum, a);
    }
    else {
        ASSERT(0 && "unhandled svc Indirect Operation");
    }
}

// note: address has already been validate()d
void SvmRuntime::addrOp(uint8_t opnum, reg_t address)
{
    switch (opnum) {
    case 0: {
        cpu.setReg(SvmCpu::REG_PC, address);
        LOG(("addrOp: long branch to 0x%x, result: 0x%x\n", address, cpu.reg(SvmCpu::REG_PC)));
        break;
    }
//    case 1:
//        LOG(("addrOp: Asynchronous preload\n"));
//        break;
//    case 2:
//        LOG(("addrOp: Assign to r8-9\n"));
//        break;
    default:
        LOG(("unknown addrOp: %d (0x%x)\n", opnum, address));
        break;
    }
}

/*
    Given an address:
    - if address is in virtual flash
        - make sure any referenced memory has been loaded.
        - translate from virtual to cached data address
    - otherwise, assume RAM address
        - if already in range of physical RAM, pass through
        - otherwise, assume virtual RAM address and translate to phys RAM
*/
reg_t SvmRuntime::validate(reg_t address)
{
    reg_t result;
    if (isFlashAddr(address)) {
        if (!inRange(address, cacheBlockBase(), FlashLayer::BLOCK_SIZE)) {
            FlashLayer::releaseRegion(flashRegion);

            uint32_t flashBlock = (address / FlashLayer::BLOCK_SIZE) * FlashLayer::BLOCK_SIZE;
            reg_t physFlashBase = flashBlock - VIRTUAL_FLASH_BASE + progInfo.textRodata.start;
            bool f = FlashLayer::getRegion(physFlashBase, FlashLayer::BLOCK_SIZE, &flashRegion);
            ASSERT(f && "validate() - couldn't retrieve new flash block\n");
        }
        result = virt2cacheFlash(address);
    }
    else {
        if (inRange(address, cpu.userRam(), SvmCpu::MEM_IN_BYTES))
            result = address;
        else
            result = virt2physRam(address);
        ASSERT(inRange(result, cpu.userRam(), SvmCpu::MEM_IN_BYTES) && "validate failed");
    }
    LOG(("validated: 0x%x for address 0x%x\n", result, address));
    setBasePtrs(result);
    return result;
}

bool SvmRuntime::loadElfFile(unsigned addr, unsigned len)
{
    FlashRegion r;
    if (!FlashLayer::getRegion(addr, FlashLayer::BLOCK_SIZE, &r)) {
        LOG(("couldn't get flash region for elf file\n"));
        return false;
    }
    uint8_t *block = static_cast<uint8_t*>(r.data());

    // verify magicality
    if ((block[Elf::EI_MAG0] != Elf::Magic0) ||
        (block[Elf::EI_MAG1] != Elf::Magic1) ||
        (block[Elf::EI_MAG2] != Elf::Magic2) ||
        (block[Elf::EI_MAG3] != Elf::Magic3))
    {
        LOG(("incorrect elf magic number"));
        return false;
    }


    Elf::FileHeader header;
    memcpy(&header, block, sizeof header);

    // ensure the file is the correct Elf version
    if (header.e_ident[Elf::EI_VERSION] != Elf::EV_CURRENT) {
        LOG(("incorrect elf file version\n"));
        return false;
    }

    // ensure the file is the correct data format
    union {
        int32_t l;
        char c[sizeof (int32_t)];
    } u;
    u.l = 1;
    if ((u.c[sizeof(int32_t) - 1] + 1) != header.e_ident[Elf::EI_DATA]) {
        LOG(("incorrect elf data format\n"));
        return false;
    }

    if (header.e_machine != Elf::EM_ARM) {
        LOG(("elf: incorrect machine format\n"));
        return false;
    }

    progInfo.entry = header.e_entry;
    /*
        We're looking for 3 segments, identified by the following profiles:
        - text segment - executable & read-only
        - data segment - read/write & non-zero size on disk
        - bss segment - read/write, zero size on disk, and non-zero size in memory
    */
    unsigned offset = header.e_phoff;
    for (unsigned i = 0; i < header.e_phnum; ++i, offset += header.e_phentsize) {
        Elf::ProgramHeader pHeader;
        memcpy(&pHeader, block + offset, header.e_phentsize);
        if (pHeader.p_type != Elf::PT_LOAD)
            continue;

        switch (pHeader.p_flags) {
        case (Elf::PF_Exec | Elf::PF_Read):
            LOG(("rodata/text segment found\n"));
            progInfo.textRodata.start = pHeader.p_offset;
            progInfo.textRodata.size = pHeader.p_filesz;
            progInfo.textRodata.vaddr = pHeader.p_vaddr;
            progInfo.textRodata.paddr = pHeader.p_paddr;
            break;
        case (Elf::PF_Read | Elf::PF_Write):
            if (pHeader.p_memsz > 0 && pHeader.p_filesz == 0) {
                LOG(("bss segment found\n"));
                progInfo.bss.start = pHeader.p_offset;
                progInfo.bss.size = pHeader.p_memsz;
            }
            else if (pHeader.p_filesz > 0) {
                LOG(("found rwdata segment\n"));
                progInfo.rwdata.start = pHeader.p_offset;
                progInfo.rwdata.size = pHeader.p_memsz;
            }
            break;
        }
    }

    FlashLayer::releaseRegion(r);
    return true;
}
