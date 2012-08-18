/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "flash_map.h"
#include "flash_volume.h"
#include "flash_volumeheader.h"
#include "flash_lfs.h"
#include "crc.h"
#include "elfprogram.h"
#include "svmloader.h"
#include "event.h"

/// Constants that affect pre-erase operations
namespace PreErase
{
    /// Minimum number of map-blocks to include in PRE_ERASED volume
    static const unsigned kMinVolumeSize = 4;

    /**
     * Maximum number of map-blocks to include in PRE_ERASED volume.
     *
     * (Too large and this may prevent our wear levelling algorithm
     * from working properly. It also may reduce the chances of
     * having a successful pre-erase happen when we may need to
     * interrupt the operation.)
     */
    static const unsigned kMaxVolumeSize = 16;

    /**
     * Maximum total number of pre-erased blocks to try and achieve
     */
    static const unsigned kMaxTotalSize = 64;
}


bool FlashVolume::isValid() const
{
    ASSERT(this);
    if (!block.isValid())
        return false;

    FlashBlockRef hdrRef;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(hdrRef, block);

    if (!hdr->isHeaderValid())
        return false;

    /*
     * Check CRCs. Note that the Map is not checked if this is
     * a deleted volume, since we selectively invalidate map entries
     * as they are recycled.
     */

    unsigned numMapEntries = hdr->numMapEntries();
    if (!FlashVolume::typeIsRecyclable(hdr->type) && hdr->crcMap != hdr->calculateMapCRC(numMapEntries))
        return false;
    if (hdr->crcErase != hdr->calculateEraseCountCRC(block, numMapEntries))
        return false;

    /*
     * Map assertions: These aren't necessary on release builds, we
     * just want to check over some of our layout assumptions during
     * testing on simulator builds.
     */

    DEBUG_ONLY({
        const FlashMap* map = hdr->getMap();

        // First map entry always describes the volume header
        ASSERT(numMapEntries >= 1);
        ASSERT(map->blocks[0].code == block.code);

        // LFS volumes always have numMapEntries==1
        ASSERT(numMapEntries == 1 || hdr->type != T_LFS);

        /*
         * Header must have the lowest block index, otherwise FlashVolumeIter
         * might find a non-header block first. (That would be a security
         * and correctness bug, since data in the middle of a volume may be
         * misinterpreted as a volume header!)
         */
        for (unsigned i = 1; i != numMapEntries; ++i) {
            FlashMapBlock b = map->blocks[i];
            ASSERT(b.isValid() == false || map->blocks[i].code > block.code);
        }
    })

    return true;
}

unsigned FlashVolume::getType() const
{
    ASSERT(isValid());
    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());
    return hdr->type;
}

FlashVolume FlashVolume::getParent() const
{
    ASSERT(isValid());
    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());
    return FlashMapBlock::fromCode(hdr->parentBlock);
}

FlashMapSpan FlashVolume::getPayload(FlashBlockRef &ref) const
{
    ASSERT(isValid());
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());

    unsigned numMapEntries = hdr->numMapEntries();
    unsigned size = hdr->payloadBlocks;
    unsigned offset = FlashVolumeHeader::payloadOffsetBlocks(numMapEntries, hdr->dataBytes);
    const FlashMap* map = hdr->getMap();

    return FlashMapSpan::create(map, offset, size);
}

uint8_t *FlashVolume::mapTypeSpecificData(FlashBlockRef &ref, unsigned &size) const
{
    /*
     * Return a pointer and size to type-specific data in our cache.
     * Restricted to only the portion of the type-specific data which
     * fits in the same cache block as the header.
     */

    ASSERT(isValid());
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());

    // Allow underflow in these calculations
    unsigned numMapEntries = hdr->numMapEntries();
    int32_t offset = hdr->dataOffsetBytes(numMapEntries);
    int32_t actualSize = FlashBlock::BLOCK_SIZE - offset;
    int32_t dataBytes = hdr->dataBytes;
    actualSize = MIN(actualSize, dataBytes);
    if (actualSize <= 0) {
        size = 0;
        return 0;
    }

    size = actualSize;
    return offset + (uint8_t*)hdr;
}

void FlashVolume::deleteSingle() const
{
    deleteSingleWithoutInvalidate();

    // Must notify LFS that we deleted a volume
    FlashLFSCache::invalidate();
}

void FlashVolume::deleteSingleWithoutInvalidate() const
{
    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
    ASSERT(hdr->isHeaderValid());

    // If we're deleting a user-visible volume, send out a change event.
    if (typeIsUserCreated(hdr->type))
        Event::setBasePending(Event::PID_BASE_VOLUME_DELETE, getHandle());

    FlashBlockWriter writer(ref);
    hdr->type = T_DELETED;
    hdr->typeCopy = T_DELETED;
}

void FlashVolume::deleteTree() const
{
    /*
     * Delete a volume and all of its children. This is equivalent to a
     * recursive delete operation, but to conserve stack space we actually
     * do this iteratively using a bit vector to keep track of pending
     * deletes.
     *
     * This also has the benefit of letting us process one complete level
     * of the deletion tree for each iteration over the volume list,
     * instead of requiring an iteration for every single delete operation.
     */

    FlashMapBlock::Set deleted;
    bool notDoneYet;
    deleted.clear();

    // This is the root of the deletion tree
    deleteSingle();
    block.mark(deleted);

    do {
        FlashVolumeIter vi;
        FlashVolume vol;

        /*
         * Visit volumes in order, first to read their parent and then to
         * potentially delete them. By doing both of these consecutively on
         * the same volume, we can reduce cache thrashing.
         *
         * To finish, we need to do a full scan in which we find no volumes
         * with deleted parents. We need this one last scan, since there's
         * no way to know if a volume we've already iterated past was parented
         * to a volume that was marked for deletion later in the same scan.
         */

        notDoneYet = false;
        vi.begin();

        while (vi.next(vol)) {
            FlashVolume parent = vol.getParent();

            if (!deleted.test(vol.block.index()) &&
                parent.block.isValid() &&
                deleted.test(parent.block.index())) {

                vol.deleteSingle();
                vol.block.mark(deleted);
                notDoneYet = true;
            }
        }
    } while (notDoneYet);
}

void FlashVolume::deleteEverything()
{
    /*
     * Delete all volumes, but of course leave erase count data in place.
     * This operation is fairly speedy, since we don't actually erase
     * any memory right away.
     */

    FlashVolumeIter vi;
    FlashVolume vol;
    vi.begin();
    while (vi.next(vol))
        vol.deleteSingle();
}

bool FlashVolumeIter::next(FlashVolume &vol)
{
    unsigned index;

    ASSERT(initialized == true);

    while (remaining.clearFirst(index)) {
        FlashVolume v(FlashMapBlock::fromIndex(index));

        if (v.isValid()) {
            FlashBlockRef ref;
            FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, v.block);
            ASSERT(hdr->isHeaderValid());
            const FlashMap *map = hdr->getMap();

            // Don't visit any future blocks that are part of this volume
            for (unsigned I = 0, E = hdr->numMapEntries(); I != E; ++I) {
                FlashMapBlock block = map->blocks[I];
                if (block.isValid())
                    block.clear(remaining);
            }

            vol = v;
            return true;
        }
    }

    return false;
}

FlashBlockRecycler::FlashBlockRecycler()
{
    ASSERT(!dirtyVolume.ref.isHeld());
    findOrphansAndDeletedVolumes();
    findCandidateVolumes();
}

void FlashBlockRecycler::findOrphansAndDeletedVolumes()
{
    /*
     * Iterate over volumes once, and calculate sets of orphan blocks,
     * deleted volumes, and calculate an average erase count.
     */

    orphanBlocks.mark();
    deletedVolumes.clear();

    uint64_t avgEraseNumerator = 0;
    uint32_t avgEraseDenominator = 0;

    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {

        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        // Remember deleted or incomplete recyclable volumes according to their header block.
        // If the volume is still mapped by SVM, skip it for now. This allows in-use volumes
        // to be deleted, knowing that they won't be recycled until they are unmapped.

        if (FlashVolume::typeIsRecyclable(hdr->type) && !SvmLoader::isVolumeMapped(vol)) {
            vol.block.mark(deletedVolumes);
        }

        // If a block is reachable at all, even by a deleted volume, it isn't orphaned.
        // Also calculate the average erase count for all mapped blocks

        unsigned numMapEntries = hdr->numMapEntries();
        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        for (unsigned I = 0; I != numMapEntries; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block.isValid()) {
                block.clear(orphanBlocks);
                avgEraseNumerator += hdr->getEraseCount(eraseRef, vol.block, I, numMapEntries);
                avgEraseDenominator++;
            }
        }
    }

    // If every block is orphaned, it's important to default to a count of zero
    averageEraseCount = avgEraseDenominator ?
        (avgEraseNumerator / avgEraseDenominator) : 0;
}

void FlashBlockRecycler::findCandidateVolumes()
{
    /*
     * Using the results of findOrphansAndDeletedVolumes(),
     * create a set of "candidate" volumes, in which at least one
     * of the valid blocks has an erase count <= the average.
     */

    candidateVolumes.clear();

    FlashMapBlock::Set iterSet = deletedVolumes;
    unsigned index;
    while (iterSet.clearFirst(index)) {

        FlashBlockRef ref;
        FlashMapBlock block = FlashMapBlock::fromIndex(index);
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, block);
        unsigned numMapEntries = hdr->numMapEntries();
        const FlashMap *map = hdr->getMap();
        FlashBlockRef eraseRef;

        ASSERT(FlashVolume(block).isValid());
        ASSERT(FlashVolume::typeIsRecyclable(hdr->type));
        ASSERT(hdr->isHeaderValid());

        for (unsigned I = 0; I != numMapEntries; ++I) {
            FlashMapBlock block = map->blocks[I];
            if (block.isValid() && hdr->getEraseCount(eraseRef, block, I, numMapEntries) <= averageEraseCount) {
                candidateVolumes.mark(index);
                break;
            }
        }
    }

    /*
     * If this set turns out to be empty for whatever reason
     * (say, we've already allocated all blocks with below-average
     * erase counts) we'll punt by making all deleted volumes into
     * candidates.
     */

    if (candidateVolumes.empty())
        candidateVolumes = deletedVolumes;
}

bool FlashBlockRecycler::next(FlashMapBlock &block, EraseCount &eraseCount)
{
    /*
     * We must start with orphaned blocks. See the explanation in the class
     * comment for FlashBlockRecycler. This part is easy- we assume they're
     * all at the average erase count.
     */

    unsigned index;
    if (orphanBlocks.clearFirst(index)) {
        block.setIndex(index);
        block.erase();
        eraseCount = averageEraseCount + 1;
        return true;
    }

    /*
     * Next, find a deleted volume to pull a block from. We use a 'candidateVolumes'
     * set in order to start working with volumes that contain blocks of a
     * below-average erase count.
     *
     * If we have a candidate volume at all, it's guaranteed to have at least
     * one recyclable block (the header).
     *
     * If we run out of viable candidate volumes, we can try to use
     * findCandidateVolumes() to refill the set. If that doesn't work,
     * we must conclude that the volume is full, and we return unsuccessfully.
     */

    FlashVolume vol;

    if (dirtyVolume.ref.isHeld()) {
        /*
         * We've already reclaimed some blocks from this deleted volume.
         * Keep working on the same volume, so we can reduce the number of
         * total writes to any volume's FlashMap.
         */
        vol = FlashMapBlock::fromAddress(dirtyVolume.ref->getAddress());
        ASSERT(vol.isValid());

    } else {
        /*
         * Use the next candidate volume, refilling the candidate set if
         * we need to.
         */

        unsigned index;
        if (!candidateVolumes.clearFirst(index)) {
            findCandidateVolumes();
            if (!candidateVolumes.clearFirst(index))
                return false;
        }
        vol = FlashMapBlock::fromIndex(index);
    }

    /*
     * Start picking arbitrary blocks from the volume. Since we're choosing
     * to wear-level only at the volume granularity (we process one deleted
     * volume at a time) it doesn't matter what order we pick them in, for
     * wear-levelling purposes at least.
     *
     * We do need to pick the volume header last, so start out iterating over
     * only non-header blocks.
     */

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
    unsigned numMapEntries = hdr->numMapEntries();
    FlashMap *map = hdr->getMap();

    for (unsigned I = 0; I != numMapEntries; ++I) {
        FlashMapBlock candidate = map->blocks[I];
        if (candidate.isValid() && candidate.code != vol.block.code) {
            // Found a non-header block to yank! Mark it as dirty.

            dirtyVolume.beginBlock(ref);
            map->blocks[I].setInvalid();

            block = candidate;
            block.erase();
            eraseCount = 1 + hdr->getEraseCount(ref, vol.block, I, numMapEntries);
            return true;
        }
    }

    // Yanking the last (header) block. Remove this volume from the pool.
    
    vol.block.clear(deletedVolumes);
    dirtyVolume.commitBlock();

    block = vol.block;
    block.erase();
    eraseCount = 1 + hdr->getEraseCount(ref, vol.block, 0, numMapEntries);
    return true;
}

bool FlashVolumeWriter::begin(unsigned type, unsigned payloadBytes,
    unsigned hdrDataBytes, FlashVolume parent)
{
    // The real type will be written in commit(), once the volume is complete.
    this->type = type;
    payloadOffset = 0;

    // Start building a volume header, in anonymous cache memory.
    // populateMap() will assign concrete block addresses to the volume.

    FlashBlockWriter writer;
    writer.beginBlock();

    unsigned payloadBlocks = (payloadBytes + FlashBlock::BLOCK_MASK) / FlashBlock::BLOCK_SIZE;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(writer.ref);
    hdr->init(FlashVolume::T_INCOMPLETE, payloadBlocks, hdrDataBytes, parent.block);

    // Fill the volume header's map with recycled memory blocks
    unsigned numMapEntries = hdr->numMapEntries();
    unsigned count = populateMap(writer, numMapEntries, volume);
    if (count == 0) {
        // Didn't allocate anything successfully, not even a header
        return false;
    }

    // Finish writing
    writer.commitBlock();
    ASSERT(volume.isValid());

    return count == numMapEntries;
}

unsigned FlashVolumeWriter::populateMap(FlashBlockWriter &hdrWriter, unsigned count, FlashVolume &hdrVolume)
{
    /*
     * Get some temporary memory to store erase counts in.
     *
     * We know that the map and header fit in one block, but the erase
     * counts have no guaranteed block-level alignment. To keep this simple,
     * we'll store them as an aligned and packed array in anonymous RAM,
     * which we'll later write into memory which may or may not overlap with
     * the header block.
     */

    FlashVolumeHeader *hdr = FlashVolumeHeader::get(hdrWriter.ref);
    ASSERT(count == hdr->numMapEntries());

    const unsigned ecPerBlock = FlashBlock::BLOCK_SIZE / sizeof(FlashVolumeHeader::EraseCount);
    const unsigned ecNumBlocks = FlashMap::NUM_MAP_BLOCKS / ecPerBlock;

    FlashBlockRef ecBlocks[ecNumBlocks];
    for (unsigned i = 0; i != ecNumBlocks; ++i)
        FlashBlock::anonymous(ecBlocks[i]);

    /*
     * Start filling both the map and the temporary erase count array.
     *
     * Note that we can fail to allocate a block at any point, but we
     * need to try our best to avoid losing any erase count data in the
     * event of an allocation failure or power loss. To preserve the
     * erase count data, we follow through with allocating a volume
     * of the T_INCOMPLETE type.
     */

    FlashMap *map = hdr->getMap();
    FlashBlockRecycler br;
    unsigned actualCount;

    for (actualCount = 0; actualCount < count; ++actualCount) {
        FlashMapBlock block;
        FlashBlockRecycler::EraseCount ec;

        if (!br.next(block, ec))
            break;

        /*
         * We must ensure that the first block has the lowest block index,
         * otherwise FlashVolumeIter might find a non-header block first.
         * (That would be a security and correctness bug, since data in the
         * middle of a volume may be misinterpreted as a volume header!)
         */

        uint32_t *ecHeader = reinterpret_cast<uint32_t*>(ecBlocks[0]->getData());
        uint32_t *ecThis = reinterpret_cast<uint32_t*>(ecBlocks[actualCount / ecPerBlock]->getData())
            + (actualCount % ecPerBlock);

        if (actualCount && block.index() < map->blocks[0].index()) {
            // New lowest block index, swap.

            map->blocks[actualCount] = map->blocks[0];
            map->blocks[0] = block;

            *ecThis = *ecHeader;
            *ecHeader = ec;

        } else {
            // Normal append

            map->blocks[actualCount] = block;
            *ecThis = ec;
        }
    }

    if (actualCount == 0) {
        // Not even successful at allocating a header!
        return 0;
    }

    // Save header address
    hdrVolume = map->blocks[0];

    /*
     * Now that we know the correct block order, we can finalize the header.
     *
     * We need to write a correct header with good CRCs, map, and erase counts
     * even if the overall populateMap() operation is not completely successful.
     * Also note that the erase counts may or may not overlap with the header
     * block. We use FlashBlockWriter to manage this complexity.
     */

    // Assign a real address to the header
    hdrWriter.relocate(hdrVolume.block.address());

    hdr->crcMap = hdr->calculateMapCRC(count);

    // Calculate erase count CRC from our anonymous memory array
    Crc32::reset();
    for (unsigned I = 0; I < count; ++I) {
        FlashBlockRecycler::EraseCount *ec;
        ec = reinterpret_cast<uint32_t*>(ecBlocks[I / ecPerBlock]->getData()) + (I % ecPerBlock);

        if (I >= actualCount) {
            // Missing block
            *ec = 0;
        }

        Crc32::add(*ec);
    }
    hdr->crcErase = Crc32::get();

    /*
     * Now start writing erase counts, which may be in different blocks.
     * Note that this implicitly releases the reference we hold to "hdr".
     * Must save dataBytes and numMapEntries before this!
     */

    unsigned dataBytes = hdr->dataBytes;

    for (unsigned I = 0; I < count; ++I) {
        FlashBlockRecycler::EraseCount *ec;
        ec = reinterpret_cast<uint32_t*>(ecBlocks[I / ecPerBlock]->getData()) + (I % ecPerBlock);
        unsigned addr = FlashVolumeHeader::eraseCountAddress(hdrVolume.block, I, count, dataBytes);
        *hdrWriter.getData<FlashBlockRecycler::EraseCount>(addr) = *ec;
    }

    return actualCount;
}

void FlashVolumeWriter::commit()
{
    // Finish writing the payload first, if necessary

    payloadWriter.commitBlock();

    // Just rewrite the header block, this time with the correct 'type'.

    FlashBlockRef ref;
    FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, volume.block);

    FlashBlockWriter writer(ref);
    hdr->setType(type);
    writer.commitBlock();

    ASSERT(volume.isValid());

    // All done. Notify userspace, if they can see this volume.
    if (volume.typeIsUserCreated(type))
        Event::setBasePending(Event::PID_BASE_VOLUME_COMMIT, volume.getHandle());
}

uint8_t *FlashVolumeWriter::mapTypeSpecificData(unsigned &size)
{
    FlashBlockRef ref;
    uint8_t *data = volume.mapTypeSpecificData(ref, size);
    payloadWriter.beginBlock(ref);
    return data;
}

void FlashVolumeWriter::appendPayload(const uint8_t *bytes, uint32_t count)
{
    FlashBlockRef spanRef;
    FlashMapSpan span = volume.getPayload(spanRef);

    while (count) {
        uint32_t chunk = count;
        FlashBlockRef dataRef;
        FlashMapSpan::PhysAddr pa;
        unsigned flags = 0;

        if ((payloadOffset & FlashBlock::BLOCK_MASK) == 0) {
            // We know this is the beginning of a fully erased block
            flags |= FlashBlock::F_KNOWN_ERASED;
        }

        if (!span.getBytes(dataRef, payloadOffset, pa, chunk, flags)) {
            // This shouldn't happen unless we're writing past the end of the span!
            ASSERT(0);
            return;
        }

        payloadWriter.beginBlock(dataRef);
        memcpy(pa, bytes, chunk);

        count -= chunk;
        payloadOffset += chunk;
        bytes += chunk;
    }
}

FlashVolume::FlashVolume(_SYSVolumeHandle vh)
{
    /*
     * Convert from _SYSVolumeHandle to a FlashVolume.
     *
     * The volume must be tested with isValid() to see if the handle
     * was actually valid at all.
     */

    if (signHandle(vh) == vh)
        block.code = vh;
    else
        block.setInvalid();
}

uint32_t FlashVolume::signHandle(uint32_t h)
{
    /*
     * This is part of hte implementation of FlashVolume::getHandle()
     * and its inverse, FlashVolume::FlashVolume(_SYSVolumeHandle).
     *
     * These are opaque 32-bit identifiers. Currently our FlashBlockMap code
     * is only 8 bits wide, so we have an extra 24 bits to play with in
     * converting FlashVolumes to userspace handles.
     *
     * In order to more strongly enforce their opacity and prevent bogus
     * FlashVolumes from getting inadvertently passed in from userspace,
     * we include a machine-specific hash in these upper bits. This function
     * replaces the upper bits of 'h' with a freshly calculated hash of
     * the lower bits, and returns the new value.
     *
     * This is not at all a secure algorithm. A savvy userspace programmer
     * could always either just brute-force attack the CRC, or they could
     * reverse-engineer the key values from a collection of valid volume
     * handles. But this will certainly prevent casual mistakes from giving
     * us bogus FlashVolumes.
     *
     * This function uses the CRC hardware.
     */

    FlashMapBlock b;
    STATIC_ASSERT(sizeof(b.code) == 1);   
    b.code = h;

    Crc32::reset();

    // Volume code
    Crc32::add(b.code);

    // Static "secret key"
    Crc32::add(0xe30f4f8e);

    // Unique hardware-specific key
    Crc32::addUniqueness();

    return b.code | (Crc32::get() & 0xFFFFFF00);
}

bool FlashVolumeWriter::beginGame(unsigned payloadBytes, const char *package)
{
    /**
     * Start writing a game, after deleting any existing game volumes with
     * the same package name.
     */

    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {
        if (vol.getType() != FlashVolume::T_GAME)
            continue;

        FlashBlockRef ref;
        Elf::Program program;
        if (program.init(vol.getPayload(ref))) {
            const char *str = program.getMetaString(ref, _SYS_METADATA_PACKAGE_STR);
            if (str && !strcmp(str, package))
                vol.deleteTree();
        }
    }

    return begin(FlashVolume::T_GAME, payloadBytes);
}

bool FlashVolumeWriter::beginLauncher(unsigned payloadBytes)
{
    /**
     * Start writing the launcher, after deleting any existing launcher.
     */

    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {
        if (vol.getType() == FlashVolume::T_LAUNCHER)
            vol.deleteTree();
    }

    return begin(FlashVolume::T_LAUNCHER, payloadBytes);
}

bool FlashVolumeWriter::preEraseBlocks()
{
    /*
     * Should we even bother? It's possible for pre-erase to do more harm than good,
     * since this does involve creating a new volume, and the associated erase and write
     * overhead.
     */

    bool anySuccess = false;
    while (FlashBlockRecycler::wantPreErase()) {

        /*
         * How big is our fake volume "payload" in order for this to cover the maximum
         * number of map blocks we're willing to put into one pre-erased volume?
         */
        unsigned payloadBlocks = PreErase::kMaxVolumeSize
            * FlashMapBlock::BLOCK_SIZE / FlashBlock::BLOCK_SIZE;

        // Build a header, and stuff it full of erased blocks.

        FlashBlockWriter writer;
        writer.beginBlock();
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(writer.ref);
        hdr->init(FlashVolume::T_PRE_ERASED, payloadBlocks, 0, FlashMapBlock::invalid());

        FlashVolume volume;
        unsigned numMapEntries = hdr->numMapEntries();
        unsigned count = populateMap(writer, numMapEntries, volume);

        // Should have allocated at least the minimum, or wantPreErase() is at fault
        ASSERT(count >= PreErase::kMinVolumeSize);
        if (!count)
            break;

        writer.commitBlock();
        ASSERT(volume.isValid());
        anySuccess = true;
    }

    return anySuccess;
}

bool FlashBlockRecycler::wantPreErase()
{
    /*
     * Preparing to pre-erase:
     *
     * First of all, we need to guess at whether this whole endeavor will
     * be worth our while. Pre-erasing has a cost, in the form of one extra
     * erase in order to store the header for a T_PRE_ERASED volume. So we
     * need to do this only if it will actually help.
     *
     * Furthermore, the block recycler will give us blocks that have been
     * pre-erased as well as blocks that haven't. So effectively, each time
     * we do this we're *moving* the pre-erased blocks to a new volume, as
     * well as collecting any new blocks that we can erase now.
     *
     * First step, we'll do a scan through the volume headers in order to
     * determine whether there are enough non-pre-erased recyclable blocks
     * in order to make this worthwhile. We will skip the pre-erase if either
     * we have too few blocks to recycle, *or* if there are already a sufficient
     * number of pre-erased blocks stowed.
     */

    // Count blocks that are pre-erased
    unsigned numPreErased = 0;

    // Blocks that are recyclable and *not* pre-erased already
    unsigned numFree = FlashMapBlock::NUM_BLOCKS;

    FlashVolumeIter vi;
    FlashVolume vol;
    vi.begin();
    while (vi.next(vol)) {
        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        if (hdr->type == FlashVolume::T_PRE_ERASED) {
            /*
             * Header block counts as recyclable, other blocks are pre-erased.
             * We need to go through and count individual blocks. Skip the
             * header block, since it is by necessity no longer erased.
             */

            const FlashMap *map = hdr->getMap();
            for (unsigned I = 1, E = hdr->numMapEntries(); I != E; ++I) {
                FlashMapBlock block = map->blocks[I];
                if (block.isValid())
                    numPreErased++;
            }

        } else if (!FlashVolume::typeIsRecyclable(hdr->type)) {
            // Not recyclable at all. Uses space.
            numFree -= hdr->numMapEntries();
        }
    }

    // Too few blocks for this to be worthwhile?
    if (numFree < PreErase::kMinVolumeSize)
        return false;

    // Already plenty of pre-erased blocks?
    if (numPreErased >= PreErase::kMaxTotalSize)
        return false;

    // Okay, let's do it!
    return true;
}
