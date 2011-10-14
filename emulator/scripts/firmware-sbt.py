#!/usr/bin/env python
#
# Simple static binary translator for our 8051 firmware. This is used
# to "bake in" a stable firmware image, so that it isn't necessary to
# have an actual cube firmware ROM in order to run the emulator.
#
# This is done to an extent for performance, but also really for code
# security. It's much, much harder to reverse engineer` a usable ROM
# image from the siftulator binary due to this transformation.
#
# Since we need some hints about basic blocks and data vs. code, we
# take the SDCC ".rst" files as input, rather than the raw hex file.
#
# M. Elizabeth Scott <beth@sifteo.com>
# 
# Copyright (c) 2011 Sifteo, Inc.
#

import sys
import binascii
import bin2c


def fixupImage(p):
    """This contains all the special-case code we apply to the compiled
       firmware during binary translation.
       """

    # Special case for the reset vector. We don't translate
    # the SDCC start-up routines. No need to, really. But
    # this means we need to stick a jump to main() in there
    # after setting up the stack.

    p.instructions[0x0000] = (
        (0x75, 0x81) + p.getSym8('__start__stack'),
        (0x02,) + p.getSym16('_main'),
        )
        
    # Our ROM palettes are actually generated machine code that
    # is jumped to when we set each palette. These never existed
    # as assembly code in the first place, so we get no mention
    # of them in the .rst file other than as data. So, we need to
    # manually annotate these.
    # 
    # Each palette is 16 bytes, consisting of 8 two-byte opcodes.

    paletteBase = p.symbols['_rom_palettes']
    for palNumber in range(16):
        addr = paletteBase + palNumber * 16
        p.branchTargets[addr] = True
        for instr in range(8):
            instrBase = addr + instr * 2
            p.instructions[instrBase] = (p.dataMemory[instrBase : instrBase+2],)


class RSTParser:
    """Read one or more .rst files, and separate their contents out into
       code, data, and labels. Store this for further translation.
       """

    ROM_SIZE = 1024 * 16

    def __init__(self):
        self.area = None
        self.dataMemory = [0] * self.ROM_SIZE
        self.instructions = {}
        self.branchTargets = {}
        self.symbols = {}
        self.contDataAddr = None

    def parseFile(self, filename):
        for line in open(filename, 'r'):
            self.parseLine(line)

    def getSym8(self, name):
        """Get a 16-bit address to the symbol, in instruction
           encoding format (big-endian bytes)
           """
        return (self.symbols[name] & 0xFF,)

    def getSym16(self, name):
        """Get a 16-bit address to the symbol, in instruction
           encoding format (big-endian bytes)
           """
        addr = self.symbols[name]
        return (addr >> 8, addr & 0xFF)

    def parseLine(self, line):
        address = line[3:7].strip()
        bytes = line[8:27].replace(' ', '').strip()
        text = line[32:].strip().split(';', 1)[0]
        tokens = text.split()

        # Is this continued data?
        if self.contDataAddr and not (address or text):
            b = binascii.a2b_hex(bytes)
            self.storeData(self.contDataAddr, b)
            self.contDataAddr = self.contDataAddr + len(b)
        else:
            self.contDataAddr = None
                
        # Normal lines
        if tokens:
     
            if tokens[0] == '.area':
                self.area = tokens[1]

            elif bytes and address and self.area in ('CONST', 'CABS'):
                a = int(address, 16)
                b = binascii.a2b_hex(bytes)
                self.storeData(a, b)

                # Data can be continued on the next line
                self.contDataAddr = a + len(b)
                
            elif bytes and address and self.area in ('CSEG', 'HOME'):
                self.storeCode(int(address, 16), binascii.a2b_hex(bytes), tokens[-1].split(',')[-1])

            elif tokens[0][-1] == ':':
                self.storeLabel(int(address, 16), tokens[0][:-1])

    def storeLabel(self, address, name):
        self.symbols[name] = address
        self.branchTargets[address] = True

    def storeData(self, address, data):
        for byte in data:
            self.dataMemory[address] = ord(byte)
            address += 1

    def storeCode(self, address, data, source):
        self.instructions[address] = (map(ord, data),)

        # Sometimes instead of using a label, targets are computed
        # relative to the current instruction's address. This is not
        # exhaustive by any means, but we need to look for at least
        # the few permutations of this that we actually use in the
        # cube firmware.

        if '.' in source:

            if source[0] == 'P':
                # Bit addressing. False alarm
                pass

            elif source == '.':
                # Current address, used in some busy-loops
                self.branchTargets[address] = True

            elif source == '(.-2)':
                # Used in SPI_WAIT
                self.branchTargets[address-2] = True

            else:
                raise Exception("Unimplemented relative label")


class CodeGenerator:
    def __init__(self, parser):
        self.p = parser
        self.opTable = opcodeTable()

    def write(self, f):
        f.write(bin2c.HEADER)
        f.write("#include <cube_cpu_opcodes.h>\n"
                "\n"
                "namespace Cube {\n"
                "namespace CPU {\n")

        self.writeCode(f)
        bin2c.writeArray(f, 'sbt_rom_data', self.p.dataMemory)

        f.write("};  // namespace CPU\n"
                "};  // namespace Cube\n")

    def beginBlock(self, f, addr):
        f.write("\tcase 0x%04x: {\n"
                "\t\tunsigned clk = 0;\n"
                % addr)

    def endBlock(self, f):
        f.write("\t\treturn clk;\n"
                "\t}\n")
        
    def writeInstruction(self, f, bytes):
        bytes += (0, 0)
        f.write("\t\tclk += Opcodes::%-20s(aCPU, 0x%02x,0x%02x,0x%02x);\n" % (
                self.opTable[bytes[0]], bytes[0], bytes[1], bytes[2]))

    def isBranch(self, bytes):
        # Really silly way to detect branches, but it works...
        mnemonic = self.opTable[bytes[0]]
        return ('j' in mnemonic or 'call' in mnemonic or 'ret' in mnemonic)

    def writeCode(self, f):
        addrs = self.p.instructions.keys()
        addrs.sort()
        inBlock = False

        f.write("\nint sbt_rom_code(em8051 *aCPU)\n"
                "{\n"
                "\tswitch (PC) {\n")

        for addr in addrs:
            # Branch targets end a basic block before emitting the instruction
            if addr in self.p.branchTargets and inBlock:
                self.endBlock(f)
                inBlock = False

            if not inBlock:
                self.beginBlock(f, addr)
                inBlock = True

            # Normally there's one instruction per address, but patches can put
            # as many instructions as they like at the same place.
            for bytes in self.p.instructions[addr]:
                self.writeInstruction(f, bytes)
                branch = self.isBranch(bytes)

            # Branches always end a basic block
            if branch and inBlock:
                self.endBlock(f)
                inBlock = False

        if inBlock:
            self.endBlock(f)

        f.write("\t}\n"
                "  aCPU->except(aCPU, EXCEPTION_SBT);\n"
                "  return 1;\n"
                "}\n")


def opcodeTable():
    t = {}
    for i in range(8):
        t[0x08 + i] = 'inc_rx'
        t[0x18 + i] = 'dec_rx'
        t[0x28 + i] = 'add_a_rx'
        t[0x38 + i] = 'addc_a_rx'
        t[0x48 + i] = 'orl_a_rx'
        t[0x58 + i] = 'anl_a_rx'
        t[0x68 + i] = 'xrl_a_rx'
        t[0x78 + i] = 'mov_rx_imm'
        t[0x88 + i] = 'mov_mem_rx'
        t[0x98 + i] = 'subb_a_rx'
        t[0xa8 + i] = 'mov_rx_mem'
        t[0xb8 + i] = 'cjne_rx_imm_offset'
        t[0xc8 + i] = 'xch_a_rx'
        t[0xd8 + i] = 'djnz_rx_offset'
        t[0xe8 + i] = 'mov_a_rx'
        t[0xf8 + i] = 'mov_rx_a'

    t[0x00] = 'nop'
    t[0x01] = 'ajmp_offset'
    t[0x02] = 'ljmp_address'
    t[0x03] = 'rr_a'
    t[0x04] = 'inc_a'
    t[0x05] = 'inc_mem'
    t[0x06] = 'inc_indir_rx'
    t[0x07] = 'inc_indir_rx'

    t[0x10] = 'jbc_bitaddr_offset'
    t[0x11] = 'acall_offset'
    t[0x12] = 'lcall_address'
    t[0x13] = 'rrc_a'
    t[0x14] = 'dec_a'
    t[0x15] = 'dec_mem'
    t[0x16] = 'dec_indir_rx'
    t[0x17] = 'dec_indir_rx'

    t[0x20] = 'jb_bitaddr_offset'
    t[0x21] = 'ajmp_offset'
    t[0x22] = 'ret'
    t[0x23] = 'rl_a'
    t[0x24] = 'add_a_imm'
    t[0x25] = 'add_a_mem'
    t[0x26] = 'add_a_indir_rx'
    t[0x27] = 'add_a_indir_rx'

    t[0x30] = 'jnb_bitaddr_offset'
    t[0x31] = 'acall_offset'
    t[0x32] = 'reti'
    t[0x33] = 'rlc_a'
    t[0x34] = 'addc_a_imm'
    t[0x35] = 'addc_a_mem'
    t[0x36] = 'addc_a_indir_rx'
    t[0x37] = 'addc_a_indir_rx'

    t[0x40] = 'jc_offset'
    t[0x41] = 'ajmp_offset'
    t[0x42] = 'orl_mem_a'
    t[0x43] = 'orl_mem_imm'
    t[0x44] = 'orl_a_imm'
    t[0x45] = 'orl_a_mem'
    t[0x46] = 'orl_a_indir_rx'
    t[0x47] = 'orl_a_indir_rx'

    t[0x50] = 'jnc_offset'
    t[0x51] = 'acall_offset'
    t[0x52] = 'anl_mem_a'
    t[0x53] = 'anl_mem_imm'
    t[0x54] = 'anl_a_imm'
    t[0x55] = 'anl_a_mem'
    t[0x56] = 'anl_a_indir_rx'
    t[0x57] = 'anl_a_indir_rx'

    t[0x60] = 'jz_offset'
    t[0x61] = 'ajmp_offset'
    t[0x62] = 'xrl_mem_a'
    t[0x63] = 'xrl_mem_imm'
    t[0x64] = 'xrl_a_imm'
    t[0x65] = 'xrl_a_mem'
    t[0x66] = 'xrl_a_indir_rx'
    t[0x67] = 'xrl_a_indir_rx'

    t[0x70] = 'jnz_offset'
    t[0x71] = 'acall_offset'
    t[0x72] = 'orl_c_bitaddr'
    t[0x73] = 'jmp_indir_a_dptr'
    t[0x74] = 'mov_a_imm'
    t[0x75] = 'mov_mem_imm'
    t[0x76] = 'mov_indir_rx_imm'
    t[0x77] = 'mov_indir_rx_imm'

    t[0x80] = 'sjmp_offset'
    t[0x81] = 'ajmp_offset'
    t[0x82] = 'anl_c_bitaddr'
    t[0x83] = 'movc_a_indir_a_pc'
    t[0x84] = 'div_ab'
    t[0x85] = 'mov_mem_mem'
    t[0x86] = 'mov_mem_indir_rx'
    t[0x87] = 'mov_mem_indir_rx'

    t[0x90] = 'mov_dptr_imm'
    t[0x91] = 'acall_offset'
    t[0x92] = 'mov_bitaddr_c'
    t[0x93] = 'movc_a_indir_a_dptr'
    t[0x94] = 'subb_a_imm'
    t[0x95] = 'subb_a_mem'
    t[0x96] = 'subb_a_indir_rx'
    t[0x97] = 'subb_a_indir_rx'

    t[0xa0] = 'orl_c_compl_bitaddr'
    t[0xa1] = 'ajmp_offset'
    t[0xa2] = 'mov_c_bitaddr'
    t[0xa3] = 'inc_dptr'
    t[0xa4] = 'mul_ab'
    t[0xa5] = 'illegal'
    t[0xa6] = 'mov_indir_rx_mem'
    t[0xa7] = 'mov_indir_rx_mem'

    t[0xb0] = 'anl_c_compl_bitaddr'
    t[0xb1] = 'acall_offset'
    t[0xb2] = 'cpl_bitaddr'
    t[0xb3] = 'cpl_c'
    t[0xb4] = 'cjne_a_imm_offset'
    t[0xb5] = 'cjne_a_mem_offset'
    t[0xb6] = 'cjne_indir_rx_imm_offset'
    t[0xb7] = 'cjne_indir_rx_imm_offset'

    t[0xc0] = 'push_mem'
    t[0xc1] = 'ajmp_offset'
    t[0xc2] = 'clr_bitaddr'
    t[0xc3] = 'clr_c'
    t[0xc4] = 'swap_a'
    t[0xc5] = 'xch_a_mem'
    t[0xc6] = 'xch_a_indir_rx'
    t[0xc7] = 'xch_a_indir_rx'

    t[0xd0] = 'pop_mem'
    t[0xd1] = 'acall_offset'
    t[0xd2] = 'setb_bitaddr'
    t[0xd3] = 'setb_c'
    t[0xd4] = 'da_a'
    t[0xd5] = 'djnz_mem_offset'
    t[0xd6] = 'xchd_a_indir_rx'
    t[0xd7] = 'xchd_a_indir_rx'

    t[0xe0] = 'movx_a_indir_dptr'
    t[0xe1] = 'ajmp_offset'
    t[0xe2] = 'movx_a_indir_rx'
    t[0xe3] = 'movx_a_indir_rx'
    t[0xe4] = 'clr_a'
    t[0xe5] = 'mov_a_mem'
    t[0xe6] = 'mov_a_indir_rx'
    t[0xe7] = 'mov_a_indir_rx'

    t[0xf0] = 'movx_indir_dptr_a'
    t[0xf1] = 'acall_offset'
    t[0xf2] = 'movx_indir_rx_a'
    t[0xf3] = 'movx_indir_rx_a'
    t[0xf4] = 'cpl_a'
    t[0xf5] = 'mov_mem_a'
    t[0xf6] = 'mov_indir_rx_a'
    t[0xf7] = 'mov_indir_rx_a'

    return t


if __name__ == '__main__':
    p = RSTParser()
    for f in sys.argv[1:]:
        p.parseFile(f)

    fixupImage(p)
    gen = CodeGenerator(p)
    gen.write(open('scripts/firmware-sbt.cpp', 'w'))
