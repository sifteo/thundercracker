#!/usr/bin/env python

import mapdata
import struct
import zlib
import binascii


class DeltaCode:
    #
    # Windowed delta encoder. Literals are two bytes, compressed codes
    # are 7-bit, with an index into a window buffer plus a delta to
    # add to that past value. Optional RLE.
    #

    rle = False
    verbose = False

    LITERAL = 0
    REF = 1
    REPEAT = 2

    def __init__(self):
        self.window = [0] * self.windowSize
        self.freq = {}
    
    def incFreq(self, symbol):
        self.freq[symbol] = self.freq.get(symbol, 0) + 1

    def pushWindow(self, word):
        self.window = [word] + self.window[:-1]

    def findCode(self, word):
        for pastIndex, pastWord in enumerate(self.window):
            diff = word - pastWord
            if diff >= self.deltaRange[0] and diff <= self.deltaRange[1]:

                code = (self.REF, pastIndex, diff)
                self.incFreq(code)
                return code

        self.incFreq(None)
        return (self.LITERAL, word)

    def encode(self, arr):
        result = []

        for i, word in enumerate(arr):
            
            code = self.findCode(word)
            replace = False

            if self.rle and len(result) >= 2 and code == result[-2]:
                # Might be a run, this code matches the second-to-last one

                if result[-1][0] == self.REPEAT and result[-1][1] < self.repeatMax:
                    # Increment
                    replace = True
                    code = (self.REPEAT, result[-1][1] + 1)
                elif result[-1] == code:
                    # Start a run
                    replace = True
                    code = (self.REPEAT, 0)

            if self.verbose:
                print "[%4d] %5d -- [%s] -- %s %-12r %s" % (
                    i, word,
                    ' '.join(["%5d" % x for x in self.window]),
                    replace and "r" or " ", code, binascii.b2a_hex(self.packCode(code)))

            if replace:
                result[-1] = code
            else:
                result.append(code)
            self.pushWindow(word)

        if self.verbose:
            self.freqSummary()

        return ''.join(map(self.packCode, result))

    def freqSummary(self):
        freq = [(v, k) for k, v in self.freq.items()]
        freq.sort()
        freq.reverse()
        
        total = 0
        for count, code in freq:
            total += count

        print
        for count, code in freq[:20]:
            print "%8d, %5.2f%% -- %r" % (count, count * 100.0 / total, code)

class DeltaCode_w2_d5(DeltaCode):
    windowSize = 1 << 2
    deltaRange = (-16, 15)

    def packCode(self, code):
        if code[0] == self.REF:
            return chr(0x80 | (code[1] << 5) | (code[2] & 0x1F))
        if code[0] == self.LITERAL:
            return struct.pack(">H", code[1])

class DeltaCode_w5_d2(DeltaCode):
    windowSize = 1 << 5
    deltaRange = (-1, 2)

    def packCode(self, code):
        if code[0] == self.REF:
            return chr(0x80 | (code[1] << 2) | (code[2] + 1))
        if code[0] == self.LITERAL:
            return struct.pack(">H", code[1])

class DeltaCode_r_w4_d2(DeltaCode):
    rle = True
    repeatMax = (1 << 6) - 1
    windowSize = 1 << 4
    deltaRange = (-1, 2)

    def packCode(self, code):
        if code[0] == self.REF:
            return chr(0x80 | (code[1] << 2) | (code[2] + 1))
        if code[0] == self.REPEAT:
            return chr(0xC0 | code[1])
        if code[0] == self.LITERAL:
            return struct.pack(">H", code[1])

class DeltaCode_d7(DeltaCode):
    windowSize = 1 << 0
    deltaRange = (-64, 63)

    def packCode(self, code):
        if code[0] == self.REF:
            return chr(0x80 | (code[2] & 0x7F))
        if code[0] == self.LITERAL:
            return struct.pack(">H", code[1])

class DeltaCode_r_d6(DeltaCode):
    rle = True
    repeatMax = (1 << 5) - 1
    windowSize = 1 << 0
    deltaRange = (-32, 31)

    def packCode(self, code):
        if code[0] == self.REF:
            return chr(0x80 | (code[2] & 0x3F))
        if code[0] == self.REPEAT:
            return chr(0xC0 | code[1])
        if code[0] == self.LITERAL:
            return struct.pack(">H", code[1])


class RLECodec4:
    """Direct translation of Stir::RLECodec4 from C++"""

    MAX_RUN = 17

    def __init__(self):
        self.runNybble = None
        self.runCount = 0
        self.isNybbleBuffered = False
        self.bufferedNybble = None

    def encode(self, nybble, out):
        if nybble != self.runNybble or self.runCount == self.MAX_RUN:
            self.encodeRun(out)

        self.runNybble = nybble
        self.runCount += 1

    def flush(self, out):
        self.encodeRun(out, True)
        if self.isNybbleBuffered:
            self.encodeNybble(0, out)

    def encodeNybble(self, value, out):
        if self.isNybbleBuffered:
            out.append(chr(self.bufferedNybble | (value << 4)))
            self.isNybbleBuffered = False
        else:
            self.bufferedNybble = value
            self.isNybbleBuffered = True

    def encodeRun(self, out, terminal=False):
        if self.runCount > 0:
            self.encodeNybble(self.runNybble, out)

            if self.runCount > 1:
                self.encodeNybble(self.runNybble, out)

                if self.runCount == 2 and not terminal:
                    # Null runs can be omitted at the end of an encoding block
                    self.encodeNybble(0, out)

                elif self.runCount > 2:
                    self.encodeNybble(self.runCount - 2, out)
        self.runCount = 0


class VariableDeltaCode:
    #
    # Delta/RLE encoder with a variable-width nybble-based scheme for
    # storing deltas. There is no literal token, instead we can store
    # deltas using the minimum number of nybbles necessary.  This is
    # reminiscent of UTF8, but with a 4-bit basic unit.  Deltas are
    # stored in an offset form. To eliminate the redundance inherent
    # in a UTF8 scheme, the delta ranges are chosen such that there is
    # only one permissible encoding for a particular delta.
    #
    #   0ddd                            8 codes         deltas [-3, 4]
    #   10dd dddd                       64 codes        deltas [-35, -4] and [5, 36]
    #   110d dddd dddd                  512 codes       deltas [-291, -36] and [37, 292]
    #   1110 dddd dddd dddd             4096 codes      deltas [-2339, -292] and [293, 2340]
    #   1111 dddd dddd dddd dddd        65536 codes     literal word values

    verbose = False

    def encode(self, arr):
        nybbles = []
        self.freq = {}
        prev = 0
        
        for i, word in enumerate(arr):
            diff = word - prev
            prev = word

            log = self.signedLog2(diff)
            self.freq[log] = self.freq.get(log, 0) + 1

            if diff < 0:
                if diff >= -3:
                    self.toNybbles(nybbles, 1, diff + 3)
                elif diff >= -35:
                    self.toNybbles(nybbles, 2, diff + 35)
                elif diff >= -291:
                    self.toNybbles(nybbles, 3, diff + 291)
                elif diff >= -2339:
                    self.toNybbles(nybbles, 4, diff + 2339)
                else:
                    self.toNybbles(nybbles, 5, word)
            else:
                if diff <= 4:
                    self.toNybbles(nybbles, 1, diff + 3)
                elif diff <= 36:
                    self.toNybbles(nybbles, 2, diff - 5 + 32)
                elif diff <= 292:
                    self.toNybbles(nybbles, 3, diff - 37 + 256)
                elif diff <= 2340:
                    self.toNybbles(nybbles, 4, diff - 293 + 2048)
                else:
                    self.toNybbles(nybbles, 5, word)

        if self.verbose:
            self.freqSummary()

        out = []
        rle = RLECodec4()
        for nyb in nybbles:
            rle.encode(nyb, out)
        rle.flush(out)
        return ''.join(out)

    def signedLog2(self, x):
        log = 0
        d = 1
        if x < 0:
            x = -x
            d = -d
        while x:
            x = x >> 1
            log = log + d
        return log

    def toNybbles(self, out, count, value):
        if count == 1:
            out.append(value & 0x7)
        elif count == 2:
            out.append(0x8 | (0x3 & (value >> 4)))
            out.append(       0xF & (value >> 0))
        elif count == 3:
            out.append(0xc | (0x1 & (value >> 8)))
            out.append(       0xF & (value >> 4))
            out.append(       0xF & (value >> 0))
        elif count == 4:
            out.append(0xe)
            out.append(       0xF & (value >> 8))
            out.append(       0xF & (value >> 4))
            out.append(       0xF & (value >> 0))
        elif count == 5:
            out.append(0xf)
            out.append(       0xF & (value >> 12))
            out.append(       0xF & (value >> 8))
            out.append(       0xF & (value >> 4))
            out.append(       0xF & (value >> 0))

    def freqSummary(self):
        freq = [(v, k) for k, v in self.freq.items()]
        freq.sort()
        freq.reverse()
        
        total = 0
        for count, code in freq:
            total += count

        print
        for count, code in freq[:20]:
            print "%8d, %5.2f%% -- %r" % (count, count * 100.0 / total, code)


def competition(name, data, *codecs):
    raw = struct.pack(">%dH" % len(data), *data)

    sizes = [
        ("raw", len(raw)),
        ]

    for c in codecs:
        print "-- %s -- %s" % (name, c.__name__)
        result = c().encode(data)
        open("result-%s-%s.bin" % (name, c.__name__), "wb").write(result)
        sizes.append((c.__name__, len(result)))

    # Try out zlib too
    for level in (1, 7, 9):
        sizes.append(("deflate-%d" % level, len(zlib.compress(raw, level))))

    print
    sizes.sort(cmp = lambda a, b: cmp(a[1], b[1]))
    for name, l in sizes:
        print "  %-18s: %6d (%6.02f%% )" % (name, l, 100 - (l * 100.0 / len(raw)))
    print 


def runTests():
    for name, data in  mapdata.data.items():
        print "\n========================== %s\n" % name
        competition(name, data,
                    DeltaCode_w2_d5,
                    DeltaCode_w5_d2,
                    DeltaCode_r_w4_d2,
                    DeltaCode_d7,
                    DeltaCode_r_d6,
                    VariableDeltaCode)



runTests()
