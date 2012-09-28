#!/usr/bin/env python

#
# Tool to verify erase log contents in a filesystem dump.
# Can operate on persistent filesystem files from the Siftulator (run with -F)
# or the output from `swiss backup` (capturing filesystem contents from hardware).
#

import sys, struct
from collections import namedtuple
import SiftulatorFlash

####################################################
# first find the erase log, and make sure there's only one
####################################################

def findEraseLog():

    eraseLog = None

    for i in range(storageFile.mcNumBlocks()):

        vol = storageFile.mcVolume(i + 1)

        if vol.isValid():
            if vol.type == SiftulatorFlash.T_ERASE_LOG:
                if eraseLog != None:
                    raise ValueError("more than one erase log!")
                eraseLog = vol

    return eraseLog

####################################################
# collect block codes for any EL_VALID records in the erase log
####################################################

def findValidErasedBlocks(eraseLog):

    payload = eraseLog.payload
    RecordSize = struct.calcsize(SiftulatorFlash.ERASELOG_RECORD_FORMAT)
    numEraseLogRecords = len(payload) / RecordSize

    EraseLogRecord = namedtuple('EraseLogRecord', 'eraseCount, blockCode, flag, check')
    erasedBlocks = []

    for i in range(numEraseLogRecords):

        offset = i * RecordSize
        recdata = payload[offset:offset + RecordSize]
        record = EraseLogRecord._make(struct.unpack(SiftulatorFlash.ERASELOG_RECORD_FORMAT, recdata))

        if record.flag == SiftulatorFlash.EL_VALID:
            print "valid erase record:", record
            erasedBlocks.append(record.blockCode)

    return erasedBlocks

####################################################
# verify all bytes are erased (0xff)
####################################################

def verifyErasedBlocks(blocks):

    for eb in blocks:
        erasedVol = storageFile.mcVolume(eb)
        for i, b in enumerate(erasedVol.rawBytes):
            if b != '\xff':
                print "!!! byte %d not erased in block %d (index %d)" % (i, eb, eb - 1)


####################################################
# main
####################################################

path = sys.argv[1]
storageFile = SiftulatorFlash.StorageFile(path)

elog = findEraseLog()
if elog == None:
    print "no erase log in this FS, sorry"
    sys.exit(0)

print "reading from erase log in block %d (index %d)" % (elog.blockCode, elog.blockCode - 1)
blocks = findValidErasedBlocks(elog)

print "verifying erased blocks are erased..."
verifyErasedBlocks(blocks)
print "done."
