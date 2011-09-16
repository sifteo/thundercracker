#!/usr/bin/env python

import mapdata
import struct
import zlib
import binascii
import sys


class Histogram:
    def __init__(self, codeSpace):
        self.hist = [0] * codeSpace
        self.total = 0

    def mark(self, code):
        self.hist[code] += 1
        self.total += 1
    
    def show(self):
        print "Histogram of %d-code space" % len(self.hist)
        for i, count in enumerate(self.hist):            
            if count:
                t = count * float(len(self.hist)) / self.total
                print "  [%4x] %8.03f  %s" % (i, t, "." * min(count, int(t * 16.0)))


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
        self.winHist = Histogram(self.windowSize)
        self.freq = {}
    
    def incFreq(self, symbol):
        self.freq[symbol] = self.freq.get(symbol, 0) + 1

    def pushWindow(self, word):
        self.window = [word] + self.window[:-1]

    def findCode(self, word):
        bestIndex = None
        bestDiff = 0x10000

        for pastIndex, pastWord in enumerate(self.window):
            diff = word - pastWord
            if diff >= self.deltaRange[0] and diff <= self.deltaRange[1]:

                if abs(diff) < abs(bestDiff):
                    bestDiff = diff
                    bestIndex = pastIndex

        if bestIndex:
            self.winHist.mark(bestIndex)
            code = (self.REF, bestIndex, bestDiff)
            self.incFreq(code)
            return code
        else:
            self.incFreq(None)
            return (self.LITERAL, word)

    def encode(self, tileW, arr):
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
            self.winHist.show()
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

class DeltaCode_r_w5_d1(DeltaCode):
    rle = True
    repeatMax = (1 << 6) - 1
    windowSize = 1 << 5
    deltaRange = (0, 1)
    
    def packCode(self, code):
        if code[0] == self.REF:
            return chr(0x80 | (code[1] << 1) | code[2])
        if code[0] == self.REPEAT:
            return chr(0xC0 | code[1])
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


class NybbleCoder:
    #
    # Base class for codecs that use a variable-length nybble-oriented codespace:
    #
    #   0ddd                            8 codes
    #   10dd dddd                       64 codes
    #   110d dddd dddd                  512 codes
    #   1110 dddd dddd dddd             4096 codes
    #   1111 dddd dddd dddd dddd        65536 codes
    #

    verbose = False
    useRLE4 = True

    def __init__(self):
        self.hist = [Histogram(5),
                     Histogram(8),
                     Histogram(64),
                     Histogram(512),
                     Histogram(4096),
                     Histogram(65536)]

    def showHist(self):
        if self.verbose:
            for h in self.hist:
                h.show()

    def packNybbles(self, nybbles):
        out = []
        rle = RLECodec4()
        for nyb in nybbles:
            if self.useRLE4:
                rle.encode(nyb, out)
            else:
                rle.encodeNybble(nyb, out)
        rle.flush(out)
        return ''.join(out)

    def toNybbles(self, out, count, value):
        self.hist[0].mark(count - 1)
        self.hist[count].mark(value)

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


class DVarNib_r4(NybbleCoder):
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
    #
    # These nybbles are then put through the same RLE4 codec we use
    # in loadstream data.

    def encode(self, tileW, arr):
        nybbles = []
        prev = 0
        
        for i, word in enumerate(arr):
            diff = word - prev
            prev = word

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

        self.showHist();
        return self.packNybbles(nybbles)


class DVarNib_2(NybbleCoder):
    #
    # This is a refinement of VariableDeltaCode. It reallocates the
    # low end of the code space, for easier access to codings for
    # small runs and sequences.
    #
    #   0ddd                            8 codes         run lengths [1,6], sequence lengths [1, 2]
    #   10dd dddd                       64 codes        deltas [-32, -1] and [2, 33]
    #   110d dddd dddd                  512 codes       deltas [-288, -33] and [34, 289]
    #   1110 dddd dddd dddd             4096 codes      deltas [-2336, -289] and [290, 2337]
    #   1111 dddd dddd dddd dddd        65536 codes     literal word values
    #

    def encode(self, tileW, arr):
        nybbles = []
        prev = 0
        code = -1
        
        for i, word in enumerate(arr):
            diff = word - prev
            prev = word

            if diff == 0 and code >= 0 and code <= 4:
                # Extend an existing run
                code += 1

            elif diff == 1 and code == 6:
                # Extend an existing sequence
                code += 1

            else:
                # Flush any buffered code
                if code >= 0:
                    self.toNybbles(nybbles, 1, code)
                    code = -1

                if diff == 0:
                    # Start a run
                    code = 0

                elif diff == 1:
                    # Start a sequence
                    code = 6

                elif diff < 0:
                    if diff >= -32:
                        self.toNybbles(nybbles, 2, diff + 32)
                    elif diff >= -288:
                        self.toNybbles(nybbles, 3, diff + 288)
                    elif diff >= -2336:
                        self.toNybbles(nybbles, 4, diff + 2336)
                    else:
                        self.toNybbles(nybbles, 5, word)
                else:
                    if diff <= 33:
                        self.toNybbles(nybbles, 2, diff - 2 + 32)
                    elif diff <= 289:
                        self.toNybbles(nybbles, 3, diff - 34 + 256)
                    elif diff <= 2337:
                        self.toNybbles(nybbles, 4, diff - 290 + 2048)
                    else:
                        self.toNybbles(nybbles, 5, word)

        # Flush any buffered code
        if code >= 0:
            self.toNybbles(nybbles, 1, code)

        self.showHist();
        return self.packNybbles(nybbles)


class DVarNib_s4(NybbleCoder):
    #
    # Four-point sampled window
    #
    # This is an attempt to combine the encoding features of DVarNib_2
    # with the best parts of a windowed codec. In datasets where
    # windowed compression gives us MUCH better results, like "title",
    # the reason windowed works so well is that it finds nearby tiles
    # with similar values. Larger window sizes are super-useful if they
    # let us find matches from vertically adjacent tiles.
    #
    # In the case of our VRAM compression codec, we really don't want
    # to spend the CPU on scanning a large window LZ77-style. So, we'd
    # like to do few comparisons. We can take advantage of the fact
    # that most matches in our windowed codecs come from tiles which
    # are very nearby. If we know about both horizontal and vertical
    # dimensions in the image, we can limit each to a few tiles. This
    # reduces the number of codes we need to use, and it keeps the
    # number of comparisons in the comppressor to a minimum.
    #
    # Here's the pattern we currently check, where X is the tile being
    # compressed, and '.' is a tile we ignore:
    #
    #  . . . . .
    #  . 3 2 . .
    #  1 0 X . .
    #  . . . . .
    #
    # The delta algorithm is generalized such that a delta is always
    # taken in reference to one of these four sampling points. So, it
    # eats two bits out of our code space.
    #
    #   0lll                            8 codes
    #     0000                                          run length 1
    #     0001                                          run length 2
    #     0010                                          run length 3
    #     0011                                          run length 4
    #     0100                                          delta -1
    #     0101                                          delta +2
    #     0110                                          sequence len 1
    #     0111                                          sequence len 2
    #
    #   10dd ddss                       64 codes,       deltas +/- 8,   [-9, -2] and [3, 10]
    #   110d dddd ddss                  512 codes       deltas +/- 64   [-73, -10] and [11, 74]
    #   1110 dddd dddd ddss             4096 codes      deltas +/- 512  [-584, -74] and [75, 585]
    #   1111 wwww wwww wwww wwww        65536 codes     literal word values
    #

    useRLE4 = False
    verbose = True

    def encode(self, tileW, arr):
        nybbles = []
        code = -1

        samplePoints = (-1, -2, -tileW, -tileW-1)
        
        for i, word in enumerate(arr):
            iterNybbles = []

            # Find the sampling point with the closest diff
            diff = 0x10000
            s = -1
            samples = [0] * len(samplePoints)
            for sI, sD in enumerate(samplePoints):
                if i + sD >= 0:
                    samples[sI] = arr[i+sD]
                    if abs(word - samples[sI]) < abs(diff):
                        diff = word - arr[i+sD]
                        s = sI

            # Allocate codes

            if s == 0 and diff == 0 and code >= 0 and code <= 2:
                # Extend an existing run
                code += 1

            elif s == 0 and diff == 1 and code == 6:
                # Extend an existing sequence
                code += 1

            else:
                # Flush any buffered code
                if code >= 0:
                    self.toNybbles(iterNybbles, 1, code)
                    code = -1

                if diff == 0 and s == 0:
                    # Start a run
                    code = 0

                elif diff == 1 and s == 0:
                    # Start a sequence
                    code = 6

                elif diff == 2 and s == 0:
                    code = 5

                elif diff == -1 and s == 0:
                    code = 4

                elif diff < 0:
                    if diff >= -9:
                        self.toNybbles(iterNybbles, 2, s | ((diff + 9) << 2))
                    elif diff >= -73:
                        self.toNybbles(iterNybbles, 3, s | ((diff + 73) << 2))
                    elif diff >= -584:
                        self.toNybbles(iterNybbles, 4, s | ((diff + 584) << 2))
                    else:
                        self.toNybbles(iterNybbles, 5, word)
                else:
                    if diff <= 10:
                        self.toNybbles(iterNybbles, 2, s | ((diff - 3 + 8) << 2))
                    elif diff <= 74:
                        self.toNybbles(iterNybbles, 3, s | ((diff - 11 + 64) << 2))
                    elif diff <= 585:
                        self.toNybbles(iterNybbles, 4, s | ((diff - 75 + 512) << 2))
                    else:
                        self.toNybbles(iterNybbles, 5, word)

            if self.verbose:
                print "[%4d] %5d -- [%s] -- %2d %5d -- %s" % (
                    i, word,
                    ' '.join(["%5d" % x for x in samples]),
                    s, diff,
                    ''.join('%x' % x for x in iterNybbles),
                    )

            nybbles.extend(iterNybbles)

        # Flush any buffered code
        if code >= 0:
            self.toNybbles(nybbles, 1, code)

        self.showHist();
        return self.packNybbles(nybbles)


def competition(name, tileW, data, *codecs):
    raw = struct.pack(">%dH" % len(data), *data)
    results = [ ("raw", raw), ]

    for c in codecs:
        print "-- %s -- %s" % (name, c.__name__)
        result = c().encode(tileW, data)
        open("result-%s-%s.bin" % (name, c.__name__), "wb").write(result)
        results.append((c.__name__, result))

    # Try out zlib too
    for level in (1, 7, 9):
        results.append(("deflate-%d" % level, zlib.compress(raw, level)))

    results.sort(cmp = lambda a, b: cmp(len(a[1]), len(b[1])))

    print
    for name, data in results:
        z = zlib.compress(data, 9)
        print "  %-18s: %6d (%6.02f%% )   gz: %6d (%6.02f%%)" % (
            name, len(data), 100 - (len(data) * 100.0 / len(raw)),
            len(z), 100 - (len(z) * 100.0 / len(raw)))
    print 


def runTests():
    for name, tileW, data in  mapdata.items:
        if len(sys.argv) < 2 or name in sys.argv:
            print "\n========================== %s\n" % name
  
            competition(name, tileW, data,
                        DeltaCode_w2_d5,
                        DeltaCode_w5_d2,
                        DeltaCode_r_w4_d2,
                        DeltaCode_r_w5_d1,
                        DeltaCode_d7,
                        DeltaCode_r_d6,
                        DVarNib_r4,
                        DVarNib_2,
                        DVarNib_s4,
                        )

runTests()
