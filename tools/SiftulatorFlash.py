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

        self.magic, self.version, self.fileSize, self.cube_count, \
            self.cube_nvmSize, self.cube_extSize, self.cube_sectorSize, \
            self.mc_pageSize, self.mc_sectorSize, self.mc_capacity, \
            self.uniqueID = struct.unpack(HEADER_FORMAT, hdr)

        if self.magic != MAGIC:
            raise ValueError("Incorrect magic number. Is this a flash storage file?\n"
                "Create a flash storage file using: `siftulator -F flash.bin game.elf`")

        if self.version != CURRENT_VERSION:
            raise ValueError("Unsupported flash storage file version!")

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


def zapSysLFSVolumes(flashImage):
    """Return a modified version of flashImage, with all SysLFS volumes deleted"""
    for vol in range(128):
        addr = vol * 0x20000
        header = flashImage[addr:addr+32]
        if header[:10] == "SiftVOL_FS" and header[-4:] == "\x00\xff\xff\xff":
            flashImage = flashImage[:addr] + ("\xff" * 32) + flashImage[addr+32:]
            print "zapping SysLFS Volume<%02x>" % (vol+1)
    return flashImage
