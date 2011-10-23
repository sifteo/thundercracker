#!/usr/bin/env python
#
# Library module used by our scripts that operate on firmware images.
#
# M. Elizabeth Scott <beth@sifteo.com>
# 
# Copyright (c) 2011 Sifteo, Inc.
#

import binascii
import os.path

ROM_SIZE = 1024 * 16

class RSTParser:
    """Read one or more .rst files, and separate their contents out into
       code, data, and labels. Store this for further translation.
       """
    def __init__(self,):
        self.area = None
        self.dataMemory = [0] * ROM_SIZE
        self.instructions = {}      # List of instructions, keyed by address of opcode
        self.branchTargets = {}     # Tracks addresses that may be branched to
        self.symbols = {}           # Map symbol name to address
        self.byteModule = {}        # For each byte, tracks the module that owns that byte
        self.patchedAddrs = {}      # Tracks instruction addresses that are being patched
        self.contDataAddr = None
        
    def parseFile(self, filename):
        module = os.path.split(filename)[-1]
        for line in open(filename, 'r'):
            self.parseLine(line, module)

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

    def parseLine(self, line, module=None):
        address = line[3:7].strip()
        bytes = line[8:27].replace(' ', '').strip()
        text = line[32:].strip().split(';', 1)[0]
        tokens = text.split()

        # Is this continued data?
        if self.contDataAddr and not (address or text):
            b = binascii.a2b_hex(bytes)
            self.storeData(self.contDataAddr, b, module)
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
                self.storeData(a, b, module)

                # Data can be continued on the next line
                self.contDataAddr = a + len(b)
                
            elif bytes and address and self.area in ('CSEG', 'HOME'):
                # Assuming this isn't one of the special addresses
                # that we patch into, store the instruction.

                a = int(address, 16)
                if a not in self.patchedAddrs:
                    self.storeCode(a, binascii.a2b_hex(bytes), tokens[-1].split(',')[-1], module)
                    
            elif bytes and address and self.area == 'GSINIT':
                # This is generated code to do static initialization.
                # To save some time (and to introduce yet another
                # difference between this binary and one that will run
                # on real hardware) we just translate these at address
                # zero.

                self.storeCode(0, binascii.a2b_hex(bytes),
                               tokens[-1].split(',')[-1], module)

            elif tokens[0][-1] == ':':
                self.storeLabel(int(address, 16), tokens[0][:-1])

    def storeLabel(self, address, name):
        self.symbols[name] = address
        self.branchTargets[address] = True

    def storeData(self, address, data, module=None):
        for byte in data:
            self.byteModule[address] = module
            self.dataMemory[address] = ord(byte)
            address += 1

    def storeCode(self, address, data, source, module=None):
        self.instructions.setdefault(address, []).append(map(ord, data))

        # Sometimes instead of using a label, targets are computed
        # relative to the current instruction's address. This is not
        # exhaustive by any means, but we need to look for at least
        # the few permutations of this that we actually use in the
        # cube firmware.

        if '.' in source:

            if len(source) > 2 and source[-2] == '.' and source[-1] in '0123456789':
                # Bit addressing. False alarm
                pass

            elif source == '.':
                # Current address, used in some busy-loops
                self.branchTargets[address] = True

            elif source == '(.-2)':
                # Used in SPI_WAIT
                self.branchTargets[address-2] = True

            else:
                raise Exception("Unimplemented relative label '%s'" % source)

        # Module size accounting, per-byte rather than per-instruction
        for byte in data:
            self.byteModule[address] = module
            address += 1

  
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

