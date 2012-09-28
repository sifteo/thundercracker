Filesystem Overview
====================

This is not a complete reference, rather just some supplemental info I've collected. There are lots of notes sprinkled around the various source and header files that provide lower level detail.

LFS (Log Structure File System) Volumes
----------------------------------------

The index is fundamentally just an array of FlashLFSIndexRecords which run parallel to the actual object data.

There are four regions of interest in a SysLFS volume:

1. The volume header (first 256-byte block) is written once, when the volume is allocated. Most of it stays constant.

2. The "meta-index" is inside the volume header, in the region reserved for volume-type-specific data. It implements a simple probabilistic filter which helps quickly rule out index blocks while searching the FS for a particular object.

3. The index, structured in 256-byte-aligned blocks, grows downward from the end of the volume's payload region

4. The object data grows upward from the beginning of the volume's payload region (immediately after the 256-byte header). Objects are 16-byte aligned, but no page alignment is required.

Within each of the 256-byte index blocks, you'll find:

1. Zero or more invalid "anchor" records. These can occur if power fails while writing an anchor record. They are all ignored while reading the index.

2. Exactly one valid "anchor". The rest of the index encodes only object sizes; the anchor is necessary to calculate the absolute address of the index block's object data.

3. Zero or more index records

An index record or an anchor can be either "valid" (CRC matches), "empty" (all 0xFF), or "invalid" (any other values). Empty records mark the beginning of the free space in the index. Valid records correspond with a real space allocation in the object data area, whereas invalid records are assumed *not* to have any corresponding allocated space, since the FS assumes that invalid records only arise if power fails during an FS write.

Below that, at address 0x0CC0200, you start seeing object data. Everything here is padded to 16-byte boundaries, but without reading the index it's hard to tell what's what.

