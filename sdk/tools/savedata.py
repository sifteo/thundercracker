#!/usr/bin/env python

#
# Template file for parsing Sifteo Base savedata files
#
# To generate a file that can be processed by this script, use
#   `swiss savedata extract <volume> <filepath>`
#
# Please feel free to use this script as the basis of your own
# save data processing - we imagine you'll want to do more than
# hex dump it :D
#

import sys, struct, uuid, array, StringIO
from collections import namedtuple

# Section Types
kHeaderSection      = 0
kRecordsSection     = 1

# Header Keys
kPackageString      = 0
kVersionString      = 1
kAppUUID            = 2
kBaseHWID           = 3
kBaseFWVersion      = 4

# SysLFS Constants & Keys
_SYS_ASSET_SLOTS_PER_BANK = 4
kAssetSlotsPerCube  = _SYS_ASSET_SLOTS_PER_BANK * 2
kNumPairings        = 24
kNumTotalAssetSlots = kNumPairings * kAssetSlotsPerCube
kNumFaults          = 16

kFaultBase          = 0x16
kPairingMRU         = kFaultBase + kNumFaults
kPairingID          = kPairingMRU + 1
kCubeBase           = kPairingID + 1
kAssetSlotBase      = kCubeBase + kNumPairings
kEnd                = kAssetSlotBase + kNumTotalAssetSlots

# File type identifier
Magic               = 0x4C4D524E74666953

def int32(f):
    return struct.unpack("<I", f.read(4))[0]

def terminatedStr(bytes):
    '''
    Extract a null terminated string from a fixed size byte array
    '''
    # probably a more succinct way of doing this...
    s = ""
    for b in bytes:
        if ord(b) == 0:
            break
        s += chr(ord(b))
    return s

def getValidSectionHeader(f):
    '''
    Sections are preceded by a header formatted as:
        <uint32_t type> <uint32_t length>
    '''
    section_type = int32(f)
    if section_type not in [kHeaderSection, kRecordsSection]:
        raise ValueError("unrecognized section type")
    section_length = int32(f)
    return section_type, section_length

def keyVal(f):
    '''
    key/value entries are formatted as:
        <uint8_t key> <uint32_t len> <len bytes of value>
    '''
    key = ord(f.read(1))
    sz = int32(f)
    return key, f.read(sz)

def readHeaders(f):
    '''
    Capture all the headers in a dict, keyed by header type
    '''
    headers = {}

    hdr_type, hdr_length = getValidSectionHeader(f)
    hdr_end = f.tell() + hdr_length
    while f.tell() < hdr_end:
        k, v = keyVal(f)
        headers[k] = header(k, v)

    return headers

def header(k, v):
    '''
    Unpack header values to their appropriate type
    '''
    if k == kAppUUID:
        return uuid.UUID(bytes=v)

    if k in [kPackageString, kVersionString, kBaseFWVersion]:
        return str(v)

    if k == kBaseHWID:
        return "".join(["%02X" % ord(b) for b in v])

def getFault(v):
    '''
    The system captures data for all faults.
    Parse the fault data into a usable structure.
    '''
    # fault struct definitions
    FaultHeaderFormat = "<HBBIQ"
    FaultHeader = namedtuple('FaultHeader', 'reference, recordType, runningVolume, code, uptime')

    FaultCubeInfoFormat = "<IIBBH"
    FaultCubeInfo = namedtuple('FaultCubeInfo', 'sysConnected, userConnected, minUserCubes, maxUserCubes, reserved')

    FaultRegistersFormat = "<III"
    FaultRegisters = namedtuple('FaultRegisters', 'pc, sp, fp, gpr')

    FaultVolumeInfoFormat = ""
    FaultVolumeInfo = namedtuple('FaultVolumeInfo', 'uuid, package, version')

    FaultMemoryDumpsFormat = ""
    FaultMemoryDumps = namedtuple('FaultMemoryDumps', 'stack, codePage')

    FaultRecordSvm = namedtuple('FaultRecordSvm', 'header, cubes, regs, vol, mem')

    # populate our tuples with the fault data
    f = StringIO.StringIO(v)

    headerData = f.read(struct.calcsize(FaultHeaderFormat))
    header = FaultHeader._make(struct.unpack(FaultHeaderFormat, headerData))

    cubeInfoData = f.read(struct.calcsize(FaultCubeInfoFormat))
    cubeInfo = FaultCubeInfo._make(struct.unpack(FaultCubeInfoFormat, cubeInfoData))

    registers = FaultRegisters(pc = int32(f),
                                sp = int32(f),
                                fp = int32(f),
                                gpr = [int32(f) for i in range(8)])
    volumeInfo = FaultVolumeInfo(uuid = uuid.UUID(bytes=f.read(16)),
                                package = terminatedStr(f.read(64)),
                                version = terminatedStr(f.read(32)))
    memoryDumps = FaultMemoryDumps(stack = f.read(256), codePage = f.read(256))

    return FaultRecordSvm(header = header, cubes = cubeInfo, regs = registers,
                            vol = volumeInfo, mem = memoryDumps)

def printFault(f):
    print "********************************"
    vol = f.vol
    print "Fault Info for %s (%s)" % (vol.package, vol.version)
    print "    uuid: %s" % vol.uuid

    hdr = f.header
    print "Header Info"
    print "    reference: %d" % hdr.reference
    print "    recordType: %d - %s" % (hdr.recordType, faultRecordTypeStr(hdr.recordType))
    print "    runningVolume: 0x%x" % hdr.runningVolume
    print "    fault code: 0x%x - %s" % (hdr.code, faultCodeStr(hdr.code))
    print "    uptime (in sys ticks): %d" % hdr.uptime

    c = f.cubes
    print "CubeInfo"
    print "    num connected to the system (bitmap): 0x%x" % c.sysConnected
    print "    num visible to user space (bitmap): 0x%x" % c.userConnected
    print "    cube range: %d - %d" % (c.minUserCubes, c.maxUserCubes)

    r = f.regs
    print "Registers"
    print "    PC: 0x%08x" % r.pc
    print "    SP: 0x%08x" % r.sp
    print "    FP: 0x%08x" % r.fp
    for i, gpr in enumerate(r.gpr):
        print "    r%d: 0x%08x" % (i, gpr)

def faultCodeStr(code):
    return {
        1: "Stack allocation failure",
        2: "Validation-time stack address error",
        3: "Branch-time code address error",
        4: "Unsupported syscall number",
        5: "Runtime load address error",
        6: "Runtime store address error",
        7: "Runtime load alignment error",
        8: "Runtime store alignment error",
        9: "Runtime code fetch error",
        10: "Runtime code alignment error",
        11: "Unhandled ARM instruction in sim",
        12: "Reserved SVC encoding",
        13: "Reserved ADDROP encoding",
        14: "User call to _SYS_abort",
        15: "Bad address in long stack LDR addrop",
        16: "Bad address in long stack STR addrop",
        17: "Bad address for async preload",
        18: "Bad saved FP value detected during return",
        19: "Memory fault while fetching _SYS_log data",
        20: "Bad address in system call",
        21: "Other bad parameter in system call",
        22: "Exception during script execution",
        23: "Bad filesystem volume handle",
        24: "Bad ELF binary header",
        25: "Bad AssetImage",
        26: "Launcher program not found",
        27: "Address in system call has insufficient alignment",
        28: "Invalid or unbound AssetSlot",
        29: "Failed to initialize read-write data segment",
        30: "Main thread is not responding",
        31: "Bad AssetConfiguration",
        32: "Incorrect AssetLoader",
    }.get(code, "Unknown error")    # default if code is not found

def faultRecordTypeStr(t):
    return {
        1: "SVM Fault Record",
    }.get(t, "Unknown fault record type")

def dumpSaveData(filepath):
    '''
    dump the contents of a savedata file to the console.
    '''
    with open(filepath, "rb") as f:

        magic = struct.unpack("<Q", f.read(8))[0]
        if magic != Magic:
            raise ValueError("this is not a savedata file")

        headers = readHeaders(f)
        for h in headers:
            print "header - key %d, val: %s" % (h, headers[h])

        # SysLFS data is system info (fault logs, etc)
        # we can format these more helpfully
        isSysLFS = headers[kPackageString] == "com.sifteo.syslfs"

        # iterate records
        records_type, records_length = getValidSectionHeader(f)
        records_end = f.tell() + records_length
        while f.tell() < records_end:
            k, v = keyVal(f)
            if isSysLFS:
                if k >= kFaultBase and k < kPairingMRU:
                    printFault(getFault(v))
            else:
                # interpret your object data as you see fit.
                # we'll just dump the hex value for now.
                valAsHex = "".join(["%02X" % ord(b) for b in v])
                print "key %d, val (%d bytes): %s" % (k, len(v), valAsHex)


if __name__ == '__main__':

    path = sys.argv[1]
    dumpSaveData(path)
