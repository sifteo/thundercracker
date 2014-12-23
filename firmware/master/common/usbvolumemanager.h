/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _USB_VOLUME_MANAGER_H
#define _USB_VOLUME_MANAGER_H

#include "usbprotocol.h"
#include "flash_volume.h"
#include "sysinfo.h"

class UsbVolumeManager
{
public:
    enum Command {
        WriteGameHeader,
        WritePayload,
        WriteCommit,
        WroteHeaderOK,
        WroteHeaderFail,
        WriteLauncherHeader,
        VolumeOverview,
        VolumeDetail,
        VolumeMetadata,
        DeleteEverything,
        DeleteVolume,
        DeleteSysLFS,
        DeleteReformat,
        FirmwareVersion,
        PairCube,
        PairingSlotDetail,
        FlashDeviceRead,
        WriteCommitOK,
        WriteCommitFail,
        BaseSysInfo,
        LFSDetail,
        WriteLFSObjectHeader,
        WriteLFSObjectHeaderFail,
        WriteLFSObjectPayload,
        DeleteLFSChildren
    };

    struct VolumeOverviewReply {
        unsigned systemBytes;
        unsigned freeBytes;
        BitVector<256> bits;
    };

    struct VolumeDetailReply {
        unsigned selfBytes;
        unsigned childBytes;
        uint16_t type;
    };

    struct VolumeMetadataRequest {
        unsigned volume;
        unsigned key;
    };

    struct LFSDetailRecord {
        unsigned address;
        unsigned size;
    };

    struct LFSDetailReply {
        unsigned count;
        // should never be this many, but this fits easily into a 64-byte USB packet
        LFSDetailRecord records[6];
    };

    struct PairCubeRequest {
        uint64_t hwid;
        unsigned pairingSlot;
    };

    struct PairingSlotDetailReply {
        uint64_t hwid;
        unsigned pairingSlot;
    };

    struct FlashDeviceReadRequest {
        uint32_t address;
        uint32_t length;
    };

    struct SysInfoReply {
        uint8_t baseUniqueID[SysInfo::UniqueIdNumBytes];
        uint8_t baseHwRevision;
        // earlier versions included only the above
        uint8_t pad;
        uint16_t pad2;
        uint32_t sysVersion;
    };

    struct LFSObjectHeader {
        _SYSVolumeHandle vh;
        unsigned key;
        uint32_t crc;
        unsigned dataSize;
    };

    static void onUsbData(const USBProtocolMsg &m);

private:
    static const unsigned SYSLFS_VOLUME_BLOCK_CODE = 0;

    struct LFSObjectWriteStatus {
        uint32_t startAddr;
        uint32_t currentAddr;
        uint32_t endAddr;
    };

    static FlashVolumeWriter writer;
    static LFSObjectWriteStatus lfsWriter;

    // handlers
    static ALWAYS_INLINE void volumeOverview(USBProtocolMsg &reply);
    static ALWAYS_INLINE void volumeDetail(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void volumeMetadata(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void lfsDetail(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void pairCube(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void pairingSlotDetail(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void flashDeviceRead(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void baseSysInfo(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void beginLFSObjectWrite(const USBProtocolMsg &m, USBProtocolMsg &reply);
    static ALWAYS_INLINE void lfsPayloadWrite(const USBProtocolMsg &m);
};

#endif // _USB_VOLUME_MANAGER_H
