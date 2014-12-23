/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
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

#ifndef BASE_DEVICE_H
#define BASE_DEVICE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

/*
 * Represents the Sifteo Base.
 * Handles requests/responses from the base over the wire.
 */

class BaseDevice
{
public:

    BaseDevice(IODevice &iodevice);

    bool beginLFSRestore(USBProtocolMsg &m, _SYSVolumeHandle vol, unsigned key, unsigned dataSize, uint32_t crc);

    const char *getFirmwareVersion(USBProtocolMsg &msg);
    const UsbVolumeManager::SysInfoReply *getBaseSysInfo(USBProtocolMsg &msg);

    UsbVolumeManager::VolumeOverviewReply *getVolumeOverview(USBProtocolMsg &msg);
    UsbVolumeManager::VolumeDetailReply *getVolumeDetail(USBProtocolMsg &msg, unsigned volBlockCode);
    bool volumeCodeForPackage(const std::string & pkg, unsigned &volBlockCode);
    UsbVolumeManager::LFSDetailReply *getLFSDetail(USBProtocolMsg &buffer, unsigned volBlockCode);

    bool pairCube(USBProtocolMsg &msg, uint64_t hwid, unsigned slot);
    UsbVolumeManager::PairingSlotDetailReply *pairingSlotDetail(USBProtocolMsg &msg, unsigned pairingSlot);

    bool requestReboot();

    bool getMetadata(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key);

    // helpers
    bool waitForReply(uint32_t header, USBProtocolMsg &msg);
    bool writeAndWaitForReply(USBProtocolMsg &msg);

private:
    IODevice &dev;
};

#endif // BASE_DEVICE_H
