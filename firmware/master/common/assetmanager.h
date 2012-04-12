/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef ASSETMANAGER_H
#define ASSETMANAGER_H

#include <stdint.h>

class AssetManager
{
public:
    AssetManager();

    static void init();
    static void onData(const uint8_t *buf, unsigned len);

private:
    enum State {
        WaitingForLength,
        WritingData
    };

    struct AssetInstallation {
        uint32_t size;              // size in bytes of this asset
        uint32_t currentAddress;    // address in external flash to write the next chunk to
        uint32_t crcword;           // if we need to store bytes of a word across chunks
        uint8_t crcwordBytes;       // how many buffered bytes of crcword are valid?
        State state;
    };

    static struct AssetInstallation installation;

    static void addToCrc(const uint8_t *buf, unsigned len);
};

#endif // ASSETMANAGER_H
