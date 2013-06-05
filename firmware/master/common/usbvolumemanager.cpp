#include "usbvolumemanager.h"
#include "elfprogram.h"
#include "bits.h"
#include "flash_volume.h"
#include "flash_volumeheader.h"
#include "flash_syslfs.h"
#include "flash_stack.h"

#ifndef SIFTEO_SIMULATOR
#include "usb/usbdevice.h"
#endif

FlashVolumeWriter UsbVolumeManager::writer;
UsbVolumeManager::LFSObjectWriteStatus UsbVolumeManager::lfsWriter;

void UsbVolumeManager::onUsbData(const USBProtocolMsg &m)
{
    USBProtocolMsg reply(USBProtocol::Installer);
    switch (m.header & 0xff) {

    case WriteGameHeader: {
        /*
         * Install a game volume, removing other games which match the same package string.
         */

        if (m.payloadLen() < 5)
            break;

        const uint32_t numBytes = *reinterpret_cast<const uint32_t*>(m.payload);
        const char* packageStr = reinterpret_cast<const char*>(m.payload + sizeof(numBytes));
        if (!memchr(packageStr, 0, m.payloadLen() - 4))
            break;

        if (writer.beginGame(numBytes, packageStr)) {
            reply.header |= WroteHeaderOK;
        } else {
            reply.header |= WroteHeaderFail;
        }
        break;
    }

    case WriteLauncherHeader: {
        /*
         * Install a launcher volume, removing other launchers first.
         */

        if (m.payloadLen() < 4)
            break;

        const uint32_t numBytes = *reinterpret_cast<const uint32_t*>(m.payload);
        if (writer.beginLauncher(numBytes)) {
            reply.header |= WroteHeaderOK;
        } else {
            reply.header |= WroteHeaderFail;
        }
        break;
    }

    case WritePayload:
        writer.appendPayload(m.payload, m.payloadLen());
        // NOTE: we don't respond to these to avoid the traffic overhead, so just return
        return;

    case WriteCommit:
        if (writer.isPayloadComplete()) {
            writer.commit();
            reply.header |= WriteCommitOK;
            reply.append(&writer.volume.block.code, 1);
        } else {
            reply.header |= WriteCommitFail;
        }
        break;

    case VolumeOverview:
        volumeOverview(reply);
        break;

    case VolumeDetail:
        volumeDetail(m, reply);
        break;

    case DeleteVolume: {
        if (m.payloadLen() < sizeof(unsigned))
            break;

        unsigned volBlockCode = *m.castPayload<unsigned>();
        FlashVolume vol = FlashMapBlock::fromCode(volBlockCode);

        if (vol.isValid()) {
            vol.deleteTree();
            reply.header |= DeleteVolume;
        }
        break;
    }

    case DeleteLFSChildren: {
        /*
         * Delete all LFS blocks that are children of the given volume.
         */
        if (m.payloadLen() < sizeof(unsigned))
            break;

        unsigned parentBlockCode = *m.castPayload<unsigned>();

        if (!FlashMapBlock::fromCode(parentBlockCode).isValid()) {
            break;
        }

        reply.header |= DeleteLFSChildren;

        FlashVolume vol;
        FlashVolumeIter vi;
        vi.begin();

        while (vi.next(vol)) {
            if (vol.getType() == FlashVolume::T_LFS &&
                vol.getParent().block.code == parentBlockCode)
            {
                vol.deleteSingle();
            }
        }
        break;
    }

    case DeleteSysLFS:
        SysLFS::deleteAll();
        reply.header |= DeleteSysLFS;
        break;

    case DeleteReformat: {
        FlashStack::reformatDevice();
        reply.header |= DeleteReformat;
        break;
    }

    case VolumeMetadata:
        volumeMetadata(m, reply);
        break;

    case DeleteEverything:
        FlashVolume::deleteEverything();
        reply.header |= DeleteEverything;
        break;

    case FirmwareVersion:
        reply.header |= FirmwareVersion;
        reply.append((const uint8_t*) TOSTRING(SDK_VERSION), sizeof(TOSTRING(SDK_VERSION)));
        break;

    case PairCube:
        pairCube(m, reply);
        break;

    case PairingSlotDetail:
        pairingSlotDetail(m, reply);
        break;

    case FlashDeviceRead:
        flashDeviceRead(m, reply);
        break;

    case BaseSysInfo:
        baseSysInfo(m, reply);
        break;

    case LFSDetail:
        lfsDetail(m, reply);
        break;

    case WriteLFSObjectHeader:
        beginLFSObjectWrite(m, reply);
        break;

    case WriteLFSObjectPayload:
        // NOTE: we don't respond to these to avoid the traffic overhead, so just return
        lfsPayloadWrite(m);
        return;
    }

#ifndef SIFTEO_SIMULATOR
    UsbDevice::write(reply.bytes, reply.len);
#endif
}

void UsbVolumeManager::volumeOverview(USBProtocolMsg &reply)
{
    /*
     * Retrieve top-level info about all volumes.
     * We treat deleted, incomplete, and LFS volumes specially here.
     */

    VolumeOverviewReply *r = reply.zeroCopyAppend<VolumeOverviewReply>();
    reply.header |= VolumeOverview;

    r->systemBytes = 0;
    r->freeBytes = FlashDevice::MAX_CAPACITY;
    r->bits.clear();

    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {
        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        // Ignore deleted/incomplete volumes. (Treat them as free space)
        if (FlashVolume::typeIsRecyclable(hdr->type))
            continue;

        // All other volumes count against our free space
        unsigned volSize = hdr->volumeSizeInBytes();
        r->freeBytes -= volSize;

        // Count space used by SysLFS
        if (hdr->parentBlock == 0 && hdr->type == FlashVolume::T_LFS) {
            r->systemBytes += volSize;
            continue;
        }

        // Other top-level volumes are marked in the bitmap
        if (hdr->parentBlock == 0)
            r->bits.mark(vol.block.code);
    }
}

void UsbVolumeManager::volumeDetail(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    /*
     * Read fixed-size details pertaining to a single volume.
     */

    if (m.payloadLen() < sizeof(unsigned))
        return;

    unsigned volBlockCode = *m.castPayload<unsigned>();
    FlashVolume vol = FlashMapBlock::fromCode(volBlockCode);

    if (vol.isValid()) {
        VolumeDetailReply *r = reply.zeroCopyAppend<VolumeDetailReply>();
        reply.header |= VolumeDetail;

        // Map the volume header
        FlashBlockRef ref;
        FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, vol.block);
        ASSERT(hdr->isHeaderValid());

        r->type = hdr->type;
        r->selfBytes = hdr->volumeSizeInBytes();
        r->childBytes = 0;

        // Total up the size of all child volumes

        FlashVolumeIter vi;
        FlashVolume child;
        vi.begin();
        while (vi.next(child)) {
            if (!FlashVolume::typeIsRecyclable(child.getType()) &&
                child.getParent().block.code == volBlockCode)
            {
                hdr = FlashVolumeHeader::get(ref, child.block);
                ASSERT(hdr->isHeaderValid());
                r->childBytes += hdr->volumeSizeInBytes();
            }
        }
    }
}

void UsbVolumeManager::volumeMetadata(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    /*
     * Read a single metadata key from a volume.
     * Returns an empty buffer on error (bad volume, not an ELF, key not found)
     */

    if (m.payloadLen() < sizeof(VolumeMetadataRequest))
        return;

    const VolumeMetadataRequest *req = m.castPayload<VolumeMetadataRequest>();

    reply.header |= VolumeMetadata;

    FlashVolume vol = FlashMapBlock::fromCode(req->volume);
    if (!vol.isValid())
        return;

    unsigned metalen = Elf::Program::copyMeta(vol, req->key, 1, sizeof reply.payload, reply.payload);
    ASSERT(metalen <= reply.MAX_PAYLOAD_BYTES);
    reply.len += metalen;
}

void UsbVolumeManager::lfsDetail(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    /*
     * Find all the LFS children of the given parent volume, and populate
     * our response with each of their details.
     */

    if (m.payloadLen() < sizeof(unsigned)) {
        return;
    }

    unsigned parentBlockCode = *m.castPayload<unsigned>();
    FlashVolume vol = FlashMapBlock::fromCode(parentBlockCode);

    LFSDetailReply *r = reply.zeroCopyAppend<LFSDetailReply>();
    r->count= 0;
    reply.header |= LFSDetail;

    // ensure the request is for SysLFS or an existing parent volume
    if (parentBlockCode != SYSLFS_VOLUME_BLOCK_CODE && !vol.isValid()) {
        return;
    }

    /*
     * Populate our overview bitmap with the indexes of all the
     * LFS children of parentBlockCode.
     */

    FlashVolumeIter vi;
    FlashVolume child;
    vi.begin();

    while (vi.next(child)) {
        if (child.getParent().block.code == parentBlockCode) {

            FlashBlockRef ref;
            FlashVolumeHeader *hdr = FlashVolumeHeader::get(ref, child.block);
            ASSERT(hdr->isHeaderValid());

            if (hdr->type == FlashVolume::T_LFS) {
                ASSERT(r->count < arraysize(r->records));
                LFSDetailRecord &rec = r->records[r->count++];
                rec.address = child.block.address();
                rec.size = FlashMapBlock::BLOCK_SIZE;
            }
        }
    }
}

void UsbVolumeManager::pairCube(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    /*
     * Directly add pairing info for the given cube hardware ID, without going
     * through the neighbor/RF based pairing process.
     *
     * Note that this will only affect the base's normal pairing process
     * after a restart, since CubeConnector only loads pairing IDs once
     * during init.
     *
     * Useful to inject pairing information at the factory, for instance.
     */

    if (m.payloadLen() < sizeof(PairCubeRequest))
        return;

    const PairCubeRequest *payload = m.castPayload<PairCubeRequest>();

    reply.header |= PairCube;

    SysLFS::PairingIDRecord rec;
    rec.load();

    if (payload->pairingSlot >= arraysize(rec.hwid)) {
        ASSERT(0);
        return;
    }

    rec.hwid[payload->pairingSlot] = payload->hwid;
    if (!SysLFS::writeObject(SysLFS::kPairingID, rec)) {
        ASSERT(0);
    }
}

void UsbVolumeManager::pairingSlotDetail(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    /*
     * Return the HWID of the requested pairing slot.
     */

    if (m.payloadLen() < sizeof(unsigned))
        return;

    unsigned pairingSlot = *m.castPayload<unsigned>();

    SysLFS::PairingIDRecord rec;
    if (pairingSlot >= arraysize(rec.hwid))
        return;

    rec.load();
    reply.header |= PairingSlotDetail;

    PairingSlotDetailReply *r = reply.zeroCopyAppend<PairingSlotDetailReply>();
    r->hwid = rec.hwid[pairingSlot];
    r->pairingSlot = pairingSlot;
}

void UsbVolumeManager::flashDeviceRead(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    if (m.payloadLen() < sizeof(FlashDeviceReadRequest))
        return;

    const FlashDeviceReadRequest *payload = m.castPayload<FlashDeviceReadRequest>();

    unsigned address = payload->address;
    unsigned length = address > FlashDevice::MAX_CAPACITY ? 0 : FlashDevice::MAX_CAPACITY - address;
    length = MIN(length, payload->length);
    length = MIN(length, reply.bytesFree());

    reply.header |= FlashDeviceRead;

    FlashDevice::read(address, reply.castPayload<uint8_t>(), length);
    reply.len += length;
}

void UsbVolumeManager::baseSysInfo(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    reply.header |= BaseSysInfo;

    SysInfoReply *r = reply.zeroCopyAppend<SysInfoReply>();
    memcpy(r->baseUniqueID, SysInfo::UniqueId, SysInfo::UniqueIdNumBytes);
    r->baseHwRevision = SysInfo::HardwareRev;
    r->sysVersion = OS_VERSION;
}

void UsbVolumeManager::beginLFSObjectWrite(const USBProtocolMsg &m, USBProtocolMsg &reply)
{
    /*
     * Begin streaming an LFS object to flash, restoring it to its parent volume.
     *
     * Perform all the setup to verify and allocate the new object - payload data
     * arrives and gets written in lfsPayloadWrite()
     *
     * This essentially works like _SYS_fs_objectWrite() - it will simply
     * add new entries for any specified keys.
     */

    if (m.payloadLen() < sizeof(LFSObjectHeader)) {
        reply.header |= WriteLFSObjectHeaderFail;
        return;
    }

    const LFSObjectHeader *payload = m.castPayload<LFSObjectHeader>();

    // Programs may only write objects in their own local volume
    FlashVolume parentVol = FlashMapBlock::fromCode(payload->vh);
    if (!parentVol.isValid()) {
        reply.header |= WriteLFSObjectHeaderFail;
        return;
    }

    if (FlashVolume::typeIsRecyclable(parentVol.getType())) {
        reply.header |= WriteLFSObjectHeaderFail;
        return;
    }

    if (!FlashLFSIndexRecord::isKeyAllowed(payload->key) ||
        !FlashLFSIndexRecord::isSizeAllowed(payload->dataSize)) {
        reply.header |= WriteLFSObjectHeaderFail;
        return;
    }

    /*
     * Allocate the LFS object. It will only become valid once we've also
     * written data to the filesystem which matches our above CRC.
     */

    FlashLFS &lfs = FlashLFSCache::get(parentVol);
    FlashLFSObjectAllocator allocator(lfs, payload->key, payload->dataSize, payload->crc);

    if (!allocator.allocateAndCollectGarbage()) {
        reply.header |= WriteLFSObjectHeaderFail;
        return;
    }

    lfsWriter.currentAddr = allocator.address();
    lfsWriter.startAddr = allocator.address();
    lfsWriter.endAddr = allocator.address() + payload->dataSize;

    reply.header |= WriteLFSObjectHeader;
}

void UsbVolumeManager::lfsPayloadWrite(const USBProtocolMsg &m)
{
    /*
     * Write an incremental chunk of LFS object data.
     */

    unsigned remaining = lfsWriter.endAddr - lfsWriter.currentAddr;
    if (remaining) {
        uint32_t chunk = MIN(remaining, m.payloadLen());

        ASSERT(chunk > 0);
        ASSERT(lfsWriter.currentAddr >= lfsWriter.startAddr);
        ASSERT(lfsWriter.currentAddr + chunk <= lfsWriter.endAddr);

        FlashDevice::write(lfsWriter.currentAddr, m.payload, chunk);

        // Verify, on siftulator only
        DEBUG_ONLY({
            uint8_t buffer[FlashLFSIndexRecord::MAX_SIZE];
            ASSERT(chunk <= sizeof buffer);
            FlashDevice::read(lfsWriter.currentAddr, buffer, chunk);
            ASSERT(0 == memcmp(buffer, m.payload, chunk));
        });

        lfsWriter.currentAddr += chunk;

        if (lfsWriter.currentAddr == lfsWriter.endAddr) {
            // If any refs are held to the page(s) we touched,
            // ensure they get reloaded from flash.
            FlashBlock::invalidate(lfsWriter.startAddr, lfsWriter.endAddr);
        }
    }
}
