/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef USBPROTOCOL_H
#define USBPROTOCOL_H

#include <stdint.h>
#include "macros.h"

struct USBProtocolMsg;

struct USBProtocol {

    enum SubSystem {
        Installer       = 0,
        FactoryTest     = 1,
        Debugger        = 2,
        Logger          = 3,
        Profiler        = 4,
        DesktopSync     = 5,
        RFPassThrough   = 6,
    };

    static void dispatch(const USBProtocolMsg &m);

    typedef void (*SubSystemHandler)(const uint8_t *buf, unsigned len);
    static const SubSystemHandler subsystemHandlers[];

};

struct USBProtocolMsg {

    static const unsigned MAX_LEN = 64;
    static const unsigned HEADER_BYTES = sizeof(uint32_t);
    static const unsigned MAX_PAYLOAD_BYTES = MAX_LEN - HEADER_BYTES;

    unsigned len;
    union {
        uint8_t bytes[MAX_LEN];
        struct {
            uint32_t header;
            uint8_t payload[MAX_PAYLOAD_BYTES];
        };
    };

    USBProtocolMsg() :
        len(0) {}

    /*
     * Highest 4 bits in the header specify the subsystem.
     */
    USBProtocolMsg(USBProtocol::SubSystem ss) :
        len(sizeof(header)), header(ss << 28) {}

    USBProtocol::SubSystem subsystem() const {
        return static_cast<USBProtocol::SubSystem>(header >> 28);
    }

    unsigned bytesFree() const {
        return sizeof(bytes) - len;
    }
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

