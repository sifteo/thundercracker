#!/usr/bin/env python
#
# A simple size profiler and static analyzer.
#
# Takes all your *.rst files as input, and generates a basic
# report on where your program bytes are going and any potential
# validation problems.
#
# Micah Elizabeth Scott <micah@misc.name>
# 
# Copyright (c) 2011-2012 Sifteo, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#

import FirmwareLib
import sys

def showSizes(d, limit=None):
    # Show sizes from a dictionary of (name, bytes) tuples.
    
    kv = d.items()
    kv.sort(lambda a,b: cmp(b[1], a[1]))
    if limit is not None:
        kv = kv[:limit]
    
    for name, size in kv:
        print "\t%-30s %5d bytes %6.2f%%" % (
            name, size, size * 100.0 / FirmwareLib.ROM_SIZE)

def profileModuleSizes(p):
    # Look at overall bytes used per module, code and data

    moduleTotals = {}
    total = 0
    
    for address, module in p.byteModule.items():
        total += 1
        moduleTotals[module] = moduleTotals.get(module, 0) + 1

    moduleTotals['(free space)'] = FirmwareLib.ROM_SIZE - total
        
    print "\nModule totals:"
    showSizes(moduleTotals)

def profileSymbolSizes(p):
    # Look at the number of bytes behind each global symbol.
    # We only look at symbols that are likely C identifiers
    # like functions. They need to start with an underscore,
    # and contain no $ signs.
    
    # Sort by increasing address
    kv = p.symbols.items()
    kv.sort(lambda a,b: cmp(a[1], b[1]))
    
    labelSizes = {}
    lastLabel = None
    lastAddr = 0
    for label, addr in kv:
        if label[0] == '_' and '$' not in label:
            labelSizes[lastLabel] = addr - lastAddr
            lastLabel = label
            lastAddr = addr
    labelSizes[lastLabel] = FirmwareLib.ROM_SIZE - lastAddr

    print "\nSymbols:"
    showSizes(labelSizes, 50)

def visualMap(p):
    numRows = 32
    numCols = 64
    bytesPerChar = 8

    print "\nVisual ROM map:\n(%d bytes per character)\n" % bytesPerChar

    moduleCodes = {}
    numAssignedCodes = 0

    for row in range(numRows):
        addr = row * (numCols * bytesPerChar)
        print "%04x: " % addr,
        for col in range(numCols):

            # Is there a consensus about which module owns this block?
            # Ignore single unallocated bytes.
            modules = []
            for byte in range(bytesPerChar):
                module = p.byteModule.get(addr)
                addr += 1
                if module:
                    modules.append(module)

            if not modules:
                module = 'None'
                code = '.'
            elif modules == [modules[0]] * len(modules):
                module = modules[0]
                code = moduleCodes.get(module, None)
                if not code:
                    code = chr(ord('A') + numAssignedCodes)
                    numAssignedCodes += 1
            else:
                module = 'Various'
                code = '-'

            moduleCodes[module] = code
            sys.stdout.write(code)
        print
    print

    kv = [(v,k) for k,v in moduleCodes.items()]
    for code, module in sorted(kv):
        print "%10s = %s" % (code, module)

def stackAnalysis(p):
    print "\nStatic RAM analysis:"

    cg = FirmwareLib.CallGraph(p)
    total = 0

    for group, (path, sp) in cg.findDeepest().iteritems():
        total = total + sp
        print "\n  Group %s, 0x%02x bytes with path:" % (group, sp)
        for node in path:
            print "    %s" % p.lines[node].strip().expandtabs()

    print "\n  Total RAM: 0x%02x bytes" % total
    if total >= 0x100:
        raise ValueError("Oh no, stack overflow is possible!")

def jumpShorteningList(p):
    print "\nLong jumps which may be shortened:"

    for addr, instrs in p.instructions.iteritems():
        for bytes in instrs:

            if len(bytes) == 3:
                target = (bytes[1] << 8) | bytes[2]
                diff = target - (addr + len(bytes))

            if bytes[0] == 0x12 and (target & 0xF800) == (addr & 0xF800):
                print "\tlcall -> acall    %s" % p.lines[addr].strip()

            if bytes[0] == 0x02:
                # Prefer ajmp to sjmp, since it's less brittle overall
                if (target & 0xF800) == (addr & 0xF800):
                    print "\tljmp  -> ajmp     %s" % p.lines[addr].strip()
                elif bytes[0] == 0x02 and diff >= -128 and diff <= 127:
                    print "\tljmp  -> sjmp     %s" % p.lines[addr].strip()

def findCriticalGadgets(p):
    # Try to defend against ROP attacks which could write to OTP memory

    print "\nCritical gadgets:\n"
    rom = ''.join(map(chr, p.rom))
    opTable = FirmwareLib.opcodeTable()

    # Critical addresses
    CRITICAL_ADDRS = [
        0xA7,   # MEMCON (allows executing code from RAM)
        0x87,   # PCON (allows read/write of program memory)
    ]

    numResults = 0

    for op in FirmwareLib.IRAM_WRITE_OPCODES:
        for ramaddr in CRITICAL_ADDRS:
            pattern = chr(op) + chr(ramaddr)
            start = 0
            while start < len(rom):
                addr = rom.find(pattern, start)
                if addr < 0:
                    break
                else:
                    start = addr + 1
                    print "\t@%04x: %02x %02x   %s" % (addr, op, ramaddr, opTable[op])
                    numResults = numResults + 1

    if numResults:
        raise ValueError("Found potential security holes")
    else:
        print "\tNone found"


if __name__ == '__main__':
    p = FirmwareLib.RSTParser()
    for f in sys.argv[1:]:
        p.parseFile(f)

    visualMap(p)
    profileModuleSizes(p)
    profileSymbolSizes(p)
    jumpShorteningList(p)
    stackAnalysis(p)
    findCriticalGadgets(p)

    print
