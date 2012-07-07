#!/usr/bin/env python

#
# Playing with using Galois fields for channel hopping...
#

def gfm(a, b):
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

def findNBitGenerators(n):
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

for n in range(1,9):
    gen, sample = findNBitGenerators(n)
    print "\n%d-bit generators in the AES Galois Field:" % n
    print " ".join("%02x" % i for i in gen)
    print "\nSample sequence:"
    print " ".join("%02x" % i for i in sample)
