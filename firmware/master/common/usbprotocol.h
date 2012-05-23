/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef USBPROTOCOL_H
#define USBPROTOCOL_H

#include <stdint.h>
#include "macros.h"

/*
 * Simple getters to help decode USB data for dispatch.
 */
class USBProtocol {
public:

    enum SubSystem {
        Installer       = 0,
        FactoryTest     = 1,
        Debugger        = 2,
        Logger          = 3,
        Profiler        = 4,
        DesktopSync     = 5,
        RFPassThrough   = 6,
    };

    typedef void (*SubSystemHandler)(const uint8_t *buf, unsigned len);

    static const unsigned HEADER_LEN = 4;

    /*
     * Headers are the first 32 bits of a message, little endian.
     */
    static inline uint32_t header(const uint8_t *buf, unsigned len) {
        ASSERT(len >= HEADER_LEN);
        return *reinterpret_cast<const uint32_t*>(buf);
    }

    /*
     * The highest 4 bits specify the subsystem.
     * Byte order is little endian.
     *
     * XXX: if the list of SubSystems doesn't grow, can we get away with 3 bits?
     */
    static inline SubSystem subsystem(const uint8_t *buf, unsigned len) {
        ASSERT(len >= HEADER_LEN);
        return static_cast<SubSystem>(buf[3] >> 4);
    }

    static void dispatch(const uint8_t *buf, unsigned len);

private:
    static const SubSystemHandler subsystemHandlers[];
};

// TODO: this is going to get killed and resurrected in its proper form sometime soon
class USBProtocolHandler {
public:
    USBProtocolHandler();

    static void init();
    static void installerHandler(const uint8_t *buf, unsigned len);

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

#endif

