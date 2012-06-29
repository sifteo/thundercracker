#!/usr/bin/env python
#
# A simple size profiler. Takes all your *.rst files as input, and
# generates a basic report on where your program bytes are going.
#
# M. Elizabeth Scott <beth@sifteo.com>
# 
# Copyright (c) 2011 Sifteo, Inc.
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


if __name__ == '__main__':
    p = FirmwareLib.RSTParser()
    for f in sys.argv[1:]:
        p.parseFile(f)

    visualMap(p)
    profileModuleSizes(p)
    profileSymbolSizes(p)
