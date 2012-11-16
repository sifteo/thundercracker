#!/usr/bin/env python

#
# Template file for parsing Sifteo Base savedata files
#
# To generate a file that can be processed by this script, use
#   `swiss savedata extract <volume> <filepath> --normalize`
#
# Please feel free to use this script as the basis of your own
# save data processing - we imagine you'll want to do more than
# hex dump it :D
#

import sys, struct, uuid

# Section Types
HeaderSection   = 0
RecordsSection  = 1

# Header Keys
PackageString   = 0
VersionString   = 1
AppUUID         = 2
BaseHWID        = 3
BaseFWVersion   = 4

# File type identifier
Magic           = 0x4C4D524E74666953

def getValidSectionHeader(f):
    '''
    Sections are preceded by a header formatted as:
        <uint32_t type> <uint32_t length>
    '''
    section_type = struct.unpack("<i", f.read(4))[0]
    if section_type not in [HeaderSection, RecordsSection]:
        raise ValueError("unrecognized section type")
    section_length = struct.unpack("<i", f.read(4))[0]
    return section_type, section_length

def keyVal(f):
    '''
    key/value entries are formatted as:
        <uint8_t key> <uint32_t len> <len bytes of value>
    '''
    key = ord(f.read(1))
    sz = struct.unpack("<i", f.read(4))[0]
    return key, f.read(sz)

def header(k, v):
    '''
    Unpack header values to their appropriate type
    '''
    if k == AppUUID:
        return uuid.UUID(bytes=v)

    if k in [PackageString, VersionString, BaseFWVersion]:
        return str(v)

    if k == BaseHWID:
        return "".join(["%02X" % ord(b) for b in v])

def dumpSaveData(filepath):
    '''
    dump the contents of a savedata file to the console.
    '''
    with open(filepath, "rb") as f:

        magic = struct.unpack("<Q", f.read(8))[0]
        if magic != Magic:
            raise ValueError("this is not a savedata file")

        # iterate headers
        hdr_type, hdr_length = getValidSectionHeader(f)
        hdr_end = f.tell() + hdr_length
        while f.tell() < hdr_end:
            k, v = keyVal(f)
            print "header - key %d, val: %s" % (k, header(k, v))

        # iterate records
        records_type, records_length = getValidSectionHeader(f)
        records_end = f.tell() + records_length
        while f.tell() < records_end:
            k, v = keyVal(f)
            # interpret your object data as you see fit.
            # we'll just dump the hex value for now.
            valAsHex = "".join(["%02X" % ord(b) for b in v])
            print "key %d, val (%d bytes): %s" % (k, len(v), valAsHex)


if __name__ == '__main__':

    path = sys.argv[1]
    dumpSaveData(path)
