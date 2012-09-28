#!/usr/bin/env python

#
# Tool for verifying erase log contents within a filesystem dump.
# Can operate on persistent filesystem files from the Siftulator (run with -F)
# or the output from swiss backup (capturing filesystem contents from hardware).
#

import sys, struct
from collections import namedtuple
import SiftulatorFlash

path = sys.argv[1]
storageFile = SiftulatorFlash.StorageFile(path)

####################################################
# first find the erase log, and make sure there's only one    
####################################################

eraseLogBlock = -1

for i in range(storageFile.mcNumBlocks()):
    
    vol = storageFile.mcVolume(i + 1)
    
    if vol.isValid():
        if vol.type == SiftulatorFlash.T_ERASE_LOG:
            
            if eraseLogBlock >= 0:
                raise ValueError("more than one erase log!")
            
            eraseLogBlock = vol.blockCode

if eraseLogBlock < 0:
    print "no erase log in this FS, sorry"
    sys.exit(0)

####################################################
# iterate through the erase log records and for any EL_VALID records,
# ensure they're actually erased
####################################################

eraseLogPayload = storageFile.mcVolume(eraseLogBlock).payload
eraseLogRecSize = struct.calcsize(SiftulatorFlash.ERASELOG_RECORD_FORMAT)
numEraseLogRecords = len(eraseLogPayload) / eraseLogRecSize

erasedBlocks = []

for i in range(numEraseLogRecords):
    
    offset = i * eraseLogRecSize
    recdata = eraseLogPayload[offset:offset + eraseLogRecSize]
    
    EraseLogRecord = namedtuple('EraseLogRecord', 'eraseCount, blockCode, flag, check')
    record = EraseLogRecord._make(struct.unpack(SiftulatorFlash.ERASELOG_RECORD_FORMAT, recdata))
        
    if record.flag == SiftulatorFlash.EL_VALID:
        print "valid erase record:", record
        erasedBlocks.append(record.blockCode)

####################################################
# verify all bytes are erased (0xff)
####################################################

print "verifying erased blocks are erased..."

for eb in erasedBlocks:
    erasedVol = storageFile.mcVolume(eb)
    for byte in erasedVol.rawBytes:
        if (byte and 0xff) != 0xff:
            print "not erased in block", eb

print "done."
