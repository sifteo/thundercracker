#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include "libusb.h"
#include "iodevice.h"

#include <list>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace Usb {

    static int init() {
        return libusb_init(NULL);
    }

    static void deinit() {
        libusb_exit(NULL);
    }

    static void setDebug(unsigned level) {

        // older versions of libusb.h don't define LIBUSB_LOG_LEVEL_DEBUG
        // so do it here to work around
        static const unsigned LOG_LEVEL_DEBUG = 4;
        if (level > LOG_LEVEL_DEBUG) {
            level = LOG_LEVEL_DEBUG;
        }
        libusb_set_debug(NULL, level);
    }

} // namespace Usb


class UsbDevice : public IODevice {
public:
    UsbDevice();

    int processEvents(unsigned timeoutMillis = 0) {
        struct timeval tv = {
            0,                      // tv_sec
            timeoutMillis * 1000    // tv_usec
        };
        return libusb_handle_events_timeout_completed(0, &tv, 0);
    }

    bool open(uint16_t vendorId, uint16_t productId, uint8_t interface = 0);
    void close();
    bool isOpen() const;

    int maxINPacketSize() const {
        return mInEndpoint.maxPacketSize;
    }

    int maxOUTPacketSize() const {
        return mOutEndpoint.maxPacketSize;
    }

    int numPendingINPackets() const {
        return mBufferedINPackets.size();
    }
    int readPacket(uint8_t *buf, unsigned maxlen, unsigned &rxlen);
    int readPacketSync(uint8_t *buf, int maxlen, int *transferred, unsigned timeout = -1);

    int numPendingOUTPackets() const {
        return mOutEndpoint.pendingTransfers.size();
    }
    int writePacket(const uint8_t *buf, unsigned len);
    int writePacketSync(const uint8_t *buf, int maxlen, int *transferred, unsigned timeout = -1);

private:
    static const unsigned NUM_CONCURRENT_IN_TRANSFERS = 16;

    bool populateDeviceInfo(libusb_config_descriptor *cfg);
    bool submitINTransfer();

    static void LIBUSB_CALL onRxComplete(libusb_transfer *);
    static void LIBUSB_CALL onTxComplete(libusb_transfer *);

    void handleRx(libusb_transfer *t);
    void handleTx(libusb_transfer *t);

    struct Endpoint {
        uint8_t address;
        uint16_t maxPacketSize;
        std::list<libusb_transfer*> pendingTransfers;
    };

    Endpoint mInEndpoint;
    Endpoint mOutEndpoint;

    void removeTransfer(Endpoint &ep, libusb_transfer *t);
    void releaseTransfers(Endpoint &ep);

    int mInterface;
    libusb_device_handle *mHandle;

    struct RxPacket {
        uint8_t *buf;
        unsigned len;
        int status;

        RxPacket(libusb_transfer *t) :
            buf((uint8_t*)malloc(t->actual_length)),
            len(t->actual_length),
            status(t->status)
        {
            memcpy(buf, t->buffer, t->actual_length);
        }
    };
    std::list<RxPacket> mBufferedINPackets;
};

#endif // _USB_DEVICE_H_
