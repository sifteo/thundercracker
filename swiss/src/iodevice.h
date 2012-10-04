#ifndef IO_DEVICE_H
#define IO_DEVICE_H

#include <stdint.h>
#include "usbprotocol.h"

/*
 * Simple backend interface.
 * Expected instances are USB for hardware, and TCP for siftulator.
 */
class IODevice {
public:
    static const unsigned MAX_EP_SIZE = 64;
    static const unsigned MAX_OUTSTANDING_OUT_TRANSFERS = 32;

    static const unsigned SIFTEO_VID = 0x22fa;
    static const unsigned BASE_PID = 0x0105;
    static const unsigned BOOTLOADER_PID = 0x0115;

    virtual bool open(uint16_t vendorId, uint16_t productId, uint8_t interface = 0) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    virtual void processEvents() = 0;

    virtual int maxINPacketSize() const = 0;
    virtual int numPendingINPackets() const = 0;
    virtual int readPacket(uint8_t *buf, unsigned maxlen) = 0;

    virtual int maxOUTPacketSize() const = 0;
    virtual int numPendingOUTPackets() const = 0;
    virtual int writePacket(const uint8_t *buf, unsigned len) = 0;

    bool writeAndWaitForReply(USBProtocolMsg &buf) {

        uint32_t headerToMatch = buf.header;
        writePacket(buf.bytes, buf.len);

        while (numPendingINPackets() == 0) {
            processEvents();
        }

        buf.len = readPacket(buf.bytes, buf.MAX_LEN);
        if (buf.header != headerToMatch) {
            fprintf(stderr, "unexpected response\n");
            return 0;
        }

        return true;
    }
};

#endif // IO_DEVICE_H
