#
# Tiny Python library for working with binary flash memory storage files
# created by Siftulator.
#
# The format of these files is defined by emulator/src/flash_storage.h
#
# M. Elizabeth Scott <beth@sifteo.com>
# Copyright <c> 2012 Sifteo, Inc. All rights reserved.
#

import struct

MAGIC = 0x534c467974666953
CURRENT_VERSION = 1

HEADER_FORMAT = "<QIIIIIIIIII"

VOL_MAGIC = 0x5f4c4f5674666953
VOL_HEADER_FORMAT = "<QHHHHHHIIBBH"

# Block Size: firmware/master/common/flash_map.h
BLOCK_SIZE      = 128 * 1024
BLOCK_MASK      = BLOCK_SIZE - 1

# Volume Types: firmware/master/common/flash_volume.h
T_LAUNCHER      = 0x4e4c
T_GAME          = 0x4e4c
T_LFS           = 0x5346
T_ERASE_LOG     = 0x4c45
T_DELETED       = 0x0000
T_INCOMPLETE    = 0xFFFF

# Erase Log info: firmware/master/common/erase_log.h
ERASELOG_RECORD_FORMAT = "<IBBH"

EL_ERASED = 0xFF
EL_POPPED = 0x00
EL_VALID  = 0x5F

class Volume:
    def __init__(self, storageFile, blockCode):
        self.blockCode = blockCode
        self.rawBytes = storageFile.mcRead((self.blockCode - 1) * BLOCK_SIZE, BLOCK_SIZE)
        self._readHeader()
        self._readPayload(storageFile)

    def _readHeader(self):

        volHdrSize = struct.calcsize(VOL_HEADER_FORMAT)
        hdr = self.rawBytes[:volHdrSize]

        self.magic,                 \
            self.type,              \
            self.payloadBlocks,     \
            self.dataBytes,         \
            self.payloadBlocksCpl,  \
            self.dataBytesCpl,      \
            self.typeCopy,          \
            self.crcMap,            \
            self.crcErase,          \
            self.parentBlock,       \
            self.parentBlockCpl,    \
            self.reserved = struct.unpack(VOL_HEADER_FORMAT, hdr)

    def _readPayload(self, storageFile):

        payloadLen = BLOCK_SIZE - storageFile.mc_pageSize
        volHdrSize = struct.calcsize(VOL_HEADER_FORMAT)

        self.payload = self.rawBytes[volHdrSize:payloadLen]

    def isValid(self):

        return self.magic == VOL_MAGIC and                                  \
                self.type == self.typeCopy and                              \
                (self.payloadBlocks ^ self.payloadBlocksCpl) == 0xFFFF and  \
                (self.dataBytes ^ self.dataBytesCpl) == 0xFFFF and          \
                (self.parentBlock ^ self.parentBlockCpl) == 0xFF

class StorageFile:
    def __init__(self, path):
        self.file = open(path, 'rb')
        self._readHeader()

    def _readHeader(self):
        size = struct.calcsize(HEADER_FORMAT)
        self.file.seek(0)
        hdr = self.file.read(size)
        if len(hdr) != size:
            raise IOError("Unexpected end-of-file while reading header")

        self.magic,                 \
            self.version,           \
            self.fileSize,          \
            self.cube_count,        \
            self.cube_nvmSize,      \
            self.cube_extSize,      \
            self.cube_sectorSize,   \
            self.mc_pageSize,       \
            self.mc_sectorSize,     \
            self.mc_capacity,       \
            self.uniqueID = struct.unpack(HEADER_FORMAT, hdr)

        if self.magic != MAGIC:
            raise ValueError("Incorrect magic number. Is this a flash storage file?\n"
                "Create a flash storage file using: `siftulator -F flash.bin game.elf`")

        if self.version != CURRENT_VERSION:
            raise ValueError("Unsupported flash storage file version!")

    def mcNumBlocks(self):
        return self.mc_capacity / BLOCK_SIZE

    def mcRead(self, offset=0, size=None):
        """Read a section of the MC filesystem. By default, reads the whole thing."""
        size = min(size or self.mc_capacity, self.mc_capacity)
        self.file.seek(offset + self.mc_pageSize)
        return self.file.read(size)

    def mcReadTruncated(self):
        """Read the entire MC filesystem, but truncate off any sectors from the
           end of the image which are fully erased (FF) or never initialized (00).

           XXX: This isn't totally correct, since it's possible that this will cut
                off a sector which is part of a valid file, it just happens to
                look empty. To do this right, we'd need to interpret the filesystem
                contents. But this is all kind of a hack anyway...
           """
        data = self.mcRead()
        emptySectors = ("\xff" * self.mc_sectorSize, "\x00" * self.mc_sectorSize)
        size = self.mc_capacity
        while (size > self.mc_sectorSize and
            data[size - self.mc_sectorSize:size] in emptySectors):
                size -= self.mc_sectorSize
        return data[:size]

    def mcVolume(self, blockCode):
        return Volume(self, blockCode)


def zapSysLFSVolumes(flashImage):
    """Return a modified version of flashImage, with all SysLFS volumes deleted"""
    for vol in range(128):
        addr = vol * 0x20000
        header = flashImage[addr:addr+32]
        if header[:10] == "SiftVOL_FS" and header[-4:] == "\x00\xff\xff\xff":
            flashImage = flashImage[:addr] + ("\xff" * 32) + flashImage[addr+32:]
            print "zapping SysLFS Volume<%02x>" % (vol+1)
    return flashImage
