#!/usr/bin/env pypy
#
# Playing with using Galois fields... We have a hardware 8-bit GF multiplier
# in the nRF chip, so we might as well use it for things. This is evaluating
# their feasibility for channel hopping and for flash CRCs.
#
# This is SUPER slow in the CPython intepreter. Using PyPy with JIT for speed.
#
# References:
#   http://www.samiam.org/galois.html
#   http://opencores.org/websvn,filedetails?repname=fast-crc&path=%2Ffast-crc%2Fweb_uploads%2FCRC_ie3_contest.pdf
#
# M. Elizabeth Scott <beth@sifteo.com>
#

import random
from hex import hexDump

# Honestly, with the current algorithm, these are all pretty much equally good.
# Note that we want one that's both a generator and which has a 1:1 mapping for CRCs.
BEST_GEN = 0x84

# Initial conditions for CRC
INITIAL = 0xff

def gfm(a, b):
    # GF(2^8) multiplier, using the AES polynomial
    p = 0
    for bit in range(8):
        if b & 1:
            p = p ^ a
        msb = a & 0x80
        a = (a << 1) & 0xff
        if msb:
            a = a ^ 0x1b
        b = b >> 1
    return p

assert gfm(7,3) == 9
assert gfm(4,3) == 0xc
assert gfm(0xe5,0xe5) == 0x4c

gfmTables = {}

def tableGFM(a, b):
    if b not in gfmTables:
        gfmTables[b] = [gfm(x, b) for x in range(256)]
    return gfmTables[b][a]


def findNBitGenerators(n):
    # A Generator is any number which, when exponentiated, hits every number
    # in the field. This function exhaustively searches for generators in
    # a field which is an N-bit subset of the AES GF(2^8) field. Note that
    # the generator itself may be a full 8-bit number, i.e. the product is
    # truncated to N bits after multiplication.

    results = []
    sample = None

    for value in range(256):
        x = value
        l = []
        r = range(1, 1<<n)
        for i in r:
            x = gfm(x, value) & ((1<<n)-1)
            l.append(x)
        if list(sorted(l)) == list(r):
            results.append(value)
            sample = sample or l

    return results, sample

def findAllGenerators():
    for n in range(2,9):
        gen, sample = findNBitGenerators(n)
        print "\n%d-bit generators in the AES Galois Field:" % n
        hexDump(gen, prefix="\t")
        print "\nSample sequence:"
        hexDump(sample, prefix="\t")

def xcrc(byte, crc, gen=BEST_GEN):
    # Experimental CRC based on galois field multiplication
    return tableGFM(crc, gen) ^ byte

def crcList(l, gen=BEST_GEN):
    reg = INITIAL
    for x in l:
        reg = xcrc(x, reg, gen)
    return reg

def testCRCBytes():
    # Try out our experimental CRC function!
    inputString = map(ord, "this is a test.....\0\0\0\0\0\1\1\1\1\1\2\2\2\2\2\3\3\3\3\3")
    crcString = []
    reg = 0
    for byte in inputString:
        reg = xcrc(byte, reg)
        crcString.append(reg)
    print "\nTest string:"
    hexDump(inputString, prefix="\t")
    print "\nCRC after each byte of test string:"
    hexDump(crcString, prefix="\t")

def bitErrorDetectionHistogram(gen, stringLen=256, numStrings=4):
    # Evaluate a given generator byte's ability to detect bit errors. Start with a random
    # string, and flip each bit. Take the CRC of each of these permutations, and calculate
    # a histogram of those CRCs. Returns that histogram.

    buckets = [0] * 256

    for n in range(numStrings):
        inputString = [random.randrange(256) for i in range(stringLen)]
        for byte in range(len(inputString)):
            for bit in range(8):
                permuted = list(inputString)
                permuted[byte] = permuted[byte] ^ (1 << bit)
                c = crcList(permuted, gen)
                buckets[c] = min(0xff, buckets[c] + 1)

    return buckets

def scoreHistogram(histogram):
    # Calculate a 'score' for how evenly distributed a histogram is
    # (Mean squared error vs. perfectly flat histogram)

    avg = sum(histogram) / float(len(histogram))
    err = 0.0
    for v in histogram:
        err += (v-avg)*(v-avg)
    return err / len(histogram)
    
def testOneToOneProperty(gen):
    # Make sure every byte hashes to a unique value
    bytes = range(256)
    hashes = [ crcList([x], gen) for x in bytes ]
    hashes.sort()
    return hashes == bytes

def testCRCBitErrors():
    print "\nEvaluating bit-error detection for generators:"
    best = None
    for gen in findNBitGenerators(8)[0]:
        if not testOneToOneProperty(gen):
            print "\t%02x: (not 1:1)" % gen
            continue

        hist = bitErrorDetectionHistogram(gen)
        score = scoreHistogram(hist)
        print "\t%02x: Score %f" % (gen, score)
        if best is None or score < best[0]:
            best = (score, gen, hist)

    print "\nHistogram for best (%02x):" % best[1]
    hexDump(best[2], prefix="\t")

def crcSamples():
    print "\nSample output for CRC:"
    assert testOneToOneProperty(BEST_GEN)
    for s in [
        "",
        "\x00",
        "\xff",
        "Hello world",
        "Hello World",
        ]:
        print "\t%-30r = %02x" % (s, crcList(map(ord, s)))

def hwidToRadioSettings(hwid):
    assert len(hwid) == 8

    reg = INITIAL
    reg = xcrc(hwid[0], reg)
    reg = xcrc(hwid[1], reg)
    reg = xcrc(hwid[2], reg)

    addr = []
    for byte in hwid[3:]:
        reg = xcrc(byte, reg)
        while 1:
            if reg in (0x00, 0xFF, 0xAA, 0x55):
                print "(disallowed byte %02x)" % reg
                reg = xcrc(0xFF, reg)
            else:
                addr.append(reg)
                break

    while 1:
        reg = xcrc(0xFF, reg)
        ch = reg & 0x7F
        if ch <= 125:
            break
        print "(disallowed channel %d)" % ch

    return tuple(addr), ch

def hexlist(bytes):
    return ''.join("%02x" % x for x in bytes)

def testHWID(hwid):
    addr, ch = hwidToRadioSettings(hwid)
    print "\t%s: %02x/%s" % (hexlist(hwid), ch, hexlist(addr))

def testRadioSettings():
    print "\nSample radio settings for various HWIDs:"

    for i in range(30):
        hwid = [random.randrange(256) for i in range(8)]
        hwid[0] = 1
        testHWID(hwid)

    for i in range(10):
        hwid[0] = i
        testHWID(hwid)

def findPairingChannels():
    print "\nFinding pairing channels:"
    for g in range(256):
        isOkay = True
        l = []
        for x in range(24, 32):
            channel = gfm(0xE0 | x, g)
            if channel in l or channel > 125:
                isOkay = False
            l.append(channel)
        if isOkay:
            m = sorted(l)
            print "\t0x%02x - Span %d, %s" % (g, m[-1] - m[0], l)

if __name__ == "__main__":
    #findAllGenerators()
    #testCRCBytes()
    #testCRCBitErrors()
    #crcSamples()
    #testRadioSettings()
    findPairingChannels()
