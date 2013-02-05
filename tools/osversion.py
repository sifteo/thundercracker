#!/usr/bin/env python

#
# Utility to generate a system version number.
# We create a numeric value of the form 0xMMNNPP (MM = major, NN = minor, PP = patch).
#
# Currently, this is derived from the git revision, but we may elect
# to change this in the future since what we're really providing is an API
# for userspace to code against, which is not strictly the same as our firmware version.
#

import subprocess, re

def bcd_byte(s):
    if len(s) > 2:
        raise ValueError("%s can't be represented in BCD, more than 2 digits" % s)

    i = 0
    for c in s:
        i <<= 4
        i |= int(c)

    return i & 0xff

if __name__ == '__main__':

    # create a uint32_t value representing the OS version based on the git tag
    # we expect tags in the form: v0.9.10-237-gcb33ed0

    githash = subprocess.check_output(["git", "describe", "--tags"]).strip()
    m = re.match('^v(\d+).(\d+).(\d+)', githash)
    if m == None:
        raise ValueError("githash (%s) didn't match expected format" % githash)

    version = 0
    for i in [1, 2, 3]:
        version <<= 8
        version |= bcd_byte(m.group(i))

    print "0x%06x" % (version)

