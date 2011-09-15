#!/usr/bin/env python

import mapdata
import struct
import zlib


#
# Codes:
#   - Literal, 14-bit
#   - Windowed delta (index, delta)
#

class Codec:
    windowSize = 4
    deltaRange = (-32, 31)

    def __init__(self):
        self.window = [0] * self.windowSize
        self.freq = {}
        self.numWords = 0
        self.encodedLen = 0
    
    def incFreq(self, symbol):
        self.freq[symbol] = self.freq.get(symbol, 0) + 1

    def pushWindow(self, word):
        self.window = [word] + self.window[:-1]

    def findCode(self, word):
        for pastIndex, pastWord in enumerate(self.window):
            diff = word - pastWord
            if diff >= self.deltaRange[0] and diff <= self.deltaRange[1]:
                return (pastIndex, diff)

    def codeLen(self, code):
        if code is None:
            # literal
            return 2
        else:
            # delta
            return 1

    def encode(self, arr, verbose=False):
        self.plaintext = arr

        for i, word in enumerate(arr):
            code = self.findCode(word)

            if i < 40:
                print "[%4d] %5d -- [%s] -- %r" % (
                    i, word,
                    ' '.join(["%5d" % x for x in self.window]),
                    code)

            self.encodedLen += self.codeLen(code)
            self.incFreq(code)
            self.numWords += 1
            self.pushWindow(word)

    def freqSummary(self):
        freq = [(v, k) for k, v in self.freq.items()]
        freq.sort()
        freq.reverse()
        
        print
        for count, code in freq[:20]:
            print "%8d, %5.2f%% -- %r" % (count, count * 100.0 / self.numWords, code)
        
    def competition(self):
        # How does this stack up against other codecs?
        
        raw = struct.pack("<%dH" % len(self.plaintext), *self.plaintext)

        sizes = [
            ("raw", len(raw)),
            ("codec", self.encodedLen),
            ]

        # Try out zlib too
        for level in range(10):
            sizes.append(("deflate-%d" % level, len(zlib.compress(raw, level))))

        print
        for name, l in sizes:
            print "  %-10s: %6d (%6.02f%% )" % (name, l, 100 - (l * 100.0 / len(raw)))
        print 


def runTests():
    for name, data in  mapdata.data.items():
        print "\n========================== %s\n" % name
        c = Codec()
        c.encode(data)
        c.freqSummary()
        c.competition()

runTests()
