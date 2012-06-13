#ifndef _USB_DEVICE_H_
#define _USB_DEVICE_H_

#include "libusb.h"
#include <queue>
#include <stdint.h>

class UsbDevice {
public:
    UsbDevice();

    static int init() {
        return libusb_init(0);
    }

    static void processEvents() {
        libusb_handle_events(0);
    }

    bool open(uint16_t vendorId, uint16_t productId, uint8_t interface = 0);
    void close();
    bool isOpen() const;

    int numPendingPackets() const {
        return mBufferedINPackets.size();
    }
    int readPacket(uint8_t *buf, unsigned maxlen);
    int readPacketSync(uint8_t *buf, int maxlen, int *transferred, unsigned timeout = -1);

    int writePacket(const uint8_t *buf, unsigned len);
    int writePacketSync(const uint8_t *buf, int maxlen, int *transferred, unsigned timeout = -1);

private:
    /*
     * Work around what appears to be a libusb bug. Only the first 3
     * active transfers successfully cancel on shutdown, so to keep
     * things simple until this is fixed, only maintain 3 outstanding IN transfers.
     */
#ifdef WIN32
    static const unsigned NUM_CONCURRENT_IN_TRANSFERS = 3;
#else
    static const unsigned NUM_CONCURRENT_IN_TRANSFERS = 16;
#endif

    bool populateDeviceInfo(libusb_device *dev);
    void openINTransfers();

    static void LIBUSB_CALL onRxComplete(libusb_transfer *);
    static void LIBUSB_CALL onTxComplete(libusb_transfer *);

    void handleRx(libusb_transfer *t);
    void handleTx(libusb_transfer *t);

    struct Endpoint {
        uint8_t address;
        uint16_t maxPacketSize;
        std::queue<libusb_transfer*> pendingTransfers;
    };

    Endpoint mInEndpoint;
    Endpoint mOutEndpoint;

    void removeTransfer(Endpoint &ep, libusb_transfer *t);

    libusb_device_handle *mHandle;

    struct Packet {
        uint8_t *buf;
        uint8_t len;
    };
    std::queue<Packet> mBufferedINPackets;
};

#endif // _USB_DEVICE_H_
