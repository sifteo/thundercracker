#!/usr/bin/env python
#
# Toys for generating graphics inner loops.
#
# M. Elizabeth Scott <beth@sifteo.com>
# Copyright <c> 2011 Sifteo, Inc. All rights reserved.
#


class MachineState:
    CTRL_LAT1 = "#CTRL_FLASH_OUT | CTRL_FLASH_LAT1"
    CTRL_LAT2 = "#CTRL_FLASH_OUT | CTRL_FLASH_LAT2"

    def __init__(self, code):
        self.ports = {}
        self.tilePtr = None
        self.code = code

    def clone(self):
        c = MachineState(self.code)
        c.ports = dict(self.ports)
        c.tilePtr = self.tilePtr
        return c

    def apply(self, otherState):
        self.setTilePtr(self.tilePtr)
        for name, value in self.ports.iteritems():
            self.setPort(name, value)

    def setPort(self, name, value):
        if type(value) is str:
            strValue = value
        else:
            strValue = "#0x%x" % value
        prev = self.ports.get(name, None)
        if prev == value:
            pass
        elif type(prev) is int and type(value) is int and prev == value - 1:
            self.code.emit('inc', name)
            self.ports[name] = value
        else:
            self.code.emit('mov', name, strValue)
            self.ports[name] = value

    def setTilePtr(self, tilePtr):
        if tilePtr != self.tilePtr:
            self.tilePtr = tilePtr
            self.ports['ADDR_PORT'] = None

            self.setPort('dptr', tilePtr)
            self.code.emit('movx', 'a', '@dptr')
            self.setPort('CTRL_PORT', self.CTRL_LAT1)

            self.setPort('dptr', tilePtr + 1)
            self.code.emit('movx', 'a', '@dptr')
            self.code.emit('mov', 'ADDR_PORT', 'a')
            self.setPort('CTRL_PORT', self.CTRL_LAT2)

    def addrTile(self, tilePtr, x, y):
        self.setTilePtr(tilePtr)
        self.setPort('ADDR_PORT', (x << 2) | (y << 5))

    def pixelBurst(self, count=1):
        for i in xrange(count * 4):
            self.setPort('ADDR_PORT', self.ports.get('ADDR_PORT', 0) + 1)


class CodeGen:
    def __init__(self):
        self.buffer = []
        self.label = 0

    def dump(self):
        print """
#include "hardware.h"

#define CHROMA_KEY  0xF5

void main() {
    __asm
"""
        for i, item in enumerate(self.buffer):
            print "\t%s\t%s" % (item[0], ', '.join(item[1:]))

        print """
    __endasm ;
}
"""

    def emit(self, *op):
        self.buffer.append(op)

    def nextLabel(self):
        l = "%d$" % self.label
        self.label += 1
        return l

    def drawLayers(self, *layers):
        state = MachineState(self)

        usingTransparency = len(layers) > 1

        for y in xrange(1):
            for x in xrange(128):

                if usingTransparency:
                    topLabel = self.nextLabel()
                    otherLabel = self.nextLabel()
                    endLabel = self.nextLabel()

                for i, layer in enumerate(layers):
                    isFirst = i == 0
                    isLast = i == len(layers) - 1

                    if isFirst:
                        # Baseline state: Top layer is opaque
                        layer.addrPixel(state, x, y)
                        ckeyState = state.clone()
                    else:
                        layer.addrPixel(ckeyState, x, y)

                    # Any layer other than the last can be transparent
                    if not isLast:
                        self.emit('mov', 'a', 'BUS_PORT')
                        if isFirst:
                            self.emit('cjne', 'a', '#CHROMA_KEY', topLabel)
                        else:
                            self.emit('cjne', 'a', '#CHROMA_KEY', otherLabel)                

                if usingTransparency:
                    # Other layer was opaque. Pixel burst, then merge state.
                    self.emit('%s:' % otherLabel)
                    state.pixelBurst()
                    state.apply(ckeyState)
                    self.emit('sjmp', endLabel)

                # Top layer was opaque. Pixel burst and get out
                if usingTransparency:
                    self.emit('%s:' % topLabel)
                state.pixelBurst()

                if usingTransparency:
                    self.emit('%s:' % endLabel)


class BG0Layer:
    size = 18 << 3
    yStride = size * 2
    xStride = 2

    def __init__(self, base=0, panX=0, panY=0):
        self.base = base
        self.panX = panX
        self.panY = panY

    def addrPixel(self, machine, x, y):
        x = (x + self.panX) % self.size
        y = (y + self.panY) % self.size

        tx = x >> 3
        ty = y >> 3
        addr = self.base + ty * self.yStride + tx * self.xStride
                
        machine.addrTile(addr, x & 7, y & 7)


c = CodeGen()
c.drawLayers(
    BG0Layer(0x000),
    BG0Layer(0x288, 2, 2),
    )
c.dump()

