#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include "libusb.h"
#include "iodevice.h"

#include <list>
#include <stdint.h>

namespace Usb {

    static int init() {
        return libusb_init(0);
    }

    static void deinit() {
        libusb_exit(0);
    }

} // namespace Usb


class UsbDevice : public IODevice {
public:
    UsbDevice();

    void processEvents() {
        struct timeval tv = {
            0,  // tv_sec
            0   // tv_usec
        };
        libusb_handle_events_timeout_completed(0, &tv, 0);
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
    int readPacket(uint8_t *buf, unsigned maxlen);
    int readPacketSync(uint8_t *buf, int maxlen, int *transferred, unsigned timeout = -1);

    int numPendingOUTPackets() const {
        return mOutEndpoint.pendingTransfers.size();
    }
    int writePacket(const uint8_t *buf, unsigned len);
    int writePacketSync(const uint8_t *buf, int maxlen, int *transferred, unsigned timeout = -1);

private:

    static const unsigned NUM_CONCURRENT_IN_TRANSFERS = 16;

    bool populateDeviceInfo(libusb_device *dev);
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

    void removeTransfer(std::list<libusb_transfer*> &list, libusb_transfer *t);

    libusb_device_handle *mHandle;

    struct Packet {
        uint8_t *buf;
        uint8_t len;
    };
    std::list<Packet> mBufferedINPackets;
};

#endif // _USB_DEVICE_H_
