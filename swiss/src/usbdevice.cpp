
#include "usbdevice.h"
#include "libusb.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#if 0
#define USB_TRACE(_x)   printf _x
#else
#define USB_TRACE(_x)
#endif


UsbDevice::UsbDevice() :
    mHandle(0)
{
}

bool UsbDevice::open(uint16_t vendorId, uint16_t productId, uint8_t interface)
{
    /*
     * Iterate all connected devices, and open the first device we see that
     * matches the given vid/pid.
     */

    USB_TRACE(("USB: Starting open()\n"));

    libusb_device **devs;
    int deviceCount = libusb_get_device_list(NULL, &devs);
    if (deviceCount < 0)
        return false;

    USB_TRACE(("USB: Walking device list\n"));

    for (int i = 0; i < deviceCount; ++i) {

        libusb_device *dev = devs[i];
        struct libusb_device_descriptor desc;
        int r = libusb_get_device_descriptor(dev, &desc);
        if (r < 0)
            continue;

        if (desc.idVendor != vendorId || desc.idProduct != productId)
            continue;

        USB_TRACE(("USB: Starting libusb_open()\n"));

        libusb_device_handle *handle;
        r = libusb_open(dev, &handle);
        if (r < 0)
            continue;

        libusb_config_descriptor *cfg;
        r = libusb_get_config_descriptor(dev, 0, &cfg);
        if (r >= 0) {
            bool found = populateDeviceInfo(cfg);
            libusb_free_config_descriptor(cfg);
            if (found) {
                mHandle = handle;
                break;
            }
        }
        libusb_close(handle);
    }

    libusb_free_device_list(devs, 1);

    if (!isOpen())
        return false;

    USB_TRACE(("USB: Claiming interface\n"));
    int r = libusb_claim_interface(mHandle, interface);
    if (r < 0)
        return false;

    /*
     * Open a number of IN transfers.
     *
     * Keeping multiple transfers open allows libusb to continue processing
     * incoming packets while we're handling recent arrivals in user space.
     */
    USB_TRACE(("USB: Submitting initial IN transfers\n"));
    for (unsigned i = 0; i < NUM_CONCURRENT_IN_TRANSFERS; ++i)
        submitINTransfer();

    USB_TRACE(("USB: Finished open()\n"));
    return true;
}

/*
 * Verify this device looks like what we expect,
 * and retrieve its endpoint details.
 */
bool UsbDevice::populateDeviceInfo(libusb_config_descriptor *cfg)
{
    USB_TRACE(("USB: Begin populateDeviceInfo()\n"));

    // need at least 1 interface
    if (cfg->bNumInterfaces < 1 || cfg->interface->num_altsetting < 1)
        return false;

    // need at least 2 endpoints
    const struct libusb_interface_descriptor *intf = cfg->interface->altsetting;

    if (intf->bNumEndpoints < 2)
        return false;

    // iterate through endpoints
    const struct libusb_endpoint_descriptor *ep = intf->endpoint;
    for (unsigned i = 0; i < intf->bNumEndpoints; ++i, ++ep) {

        if (ep->bEndpointAddress & LIBUSB_ENDPOINT_IN) {
            mInEndpoint.address = ep->bEndpointAddress;
            mInEndpoint.maxPacketSize = ep->wMaxPacketSize;
        } else {
            mOutEndpoint.address = ep->bEndpointAddress;
            mOutEndpoint.maxPacketSize = ep->wMaxPacketSize;
        }

    }

    USB_TRACE(("USB: Finished populateDeviceInfo()\n"));
    return true;
}


bool UsbDevice::submitINTransfer()
{
    USB_TRACE(("USB: Submit IN\n"));

    libusb_transfer *txfer = libusb_alloc_transfer(0);
    if (!txfer)
        return false;

    unsigned char *buf = (unsigned char*)malloc(mInEndpoint.maxPacketSize);
    if (!buf)
        return false;

    libusb_fill_bulk_transfer(txfer, mHandle, mInEndpoint.address, buf, mInEndpoint.maxPacketSize, onRxComplete, this, 0);

    int r = libusb_submit_transfer(txfer);
    if (r < 0) {
        free(buf);
        return false;
    }

    mInEndpoint.pendingTransfers.push_back(txfer);
    return true;
}

void UsbDevice::close()
{
    USB_TRACE(("USB: Closing device\n"));

    if (!isOpen())
        return;

    for (std::list<libusb_transfer*>::iterator i = mInEndpoint.pendingTransfers.begin();
         i != mInEndpoint.pendingTransfers.end(); ++i) {
        libusb_cancel_transfer(*i);
    }

    for (std::list<libusb_transfer*>::iterator i = mOutEndpoint.pendingTransfers.begin();
         i != mOutEndpoint.pendingTransfers.end(); ++i) {
        libusb_cancel_transfer(*i);
    }
}

bool UsbDevice::isOpen() const
{
    return mHandle != 0;
}

int UsbDevice::readPacket(uint8_t *buf, unsigned maxlen)
{
    assert(!mBufferedINPackets.empty());

    Packet pkt = mBufferedINPackets.front();
    uint8_t len = maxlen < pkt.len ? maxlen : pkt.len;
    memcpy(buf, pkt.buf, len);

    delete pkt.buf;
    mBufferedINPackets.pop_front();

    USB_TRACE(("USB: Read %d bytes, %02x%02x%02x%02x %02x%02x%02x%02x ...\n",
        len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));

    return len;
}

int UsbDevice::readPacketSync(uint8_t *buf, int maxlen, int *transferred, unsigned timeout)
{
    return libusb_bulk_transfer(mHandle, mInEndpoint.address, buf, maxlen, transferred, timeout);
}

int UsbDevice::writePacket(const uint8_t *buf, unsigned len)
{
    USB_TRACE(("USB: Write %d bytes, %02x%02x%02x%02x %02x%02x%02x%02x ...\n",
        len, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));

    libusb_transfer *txfer = libusb_alloc_transfer(0);
    if (!txfer) {
        return -1;
    }

    int size = len < mOutEndpoint.maxPacketSize ? len : mOutEndpoint.maxPacketSize;
    unsigned char *transferBuf = (unsigned char*)malloc(size);
    if (!transferBuf) {
        return -1;
    }
    memcpy(transferBuf, buf, size);

    libusb_fill_bulk_transfer(txfer, mHandle, mOutEndpoint.address, transferBuf, size, onTxComplete, this, 0);

    int r = libusb_submit_transfer(txfer);
    if (r < 0) {
        return -1;
    }

    mOutEndpoint.pendingTransfers.push_back(txfer);
    return size;
}

int UsbDevice::writePacketSync(const uint8_t *buf, int maxlen, int *transferred, unsigned timeout)
{
    USB_TRACE(("USB: Sync write %d bytes, %02x%02x%02x%02x %02x%02x%02x%02x ...\n",
        maxlen, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));

    return libusb_bulk_transfer(mHandle, mOutEndpoint.address, (unsigned char*)buf, maxlen, transferred, timeout);
}

/*
 * Called back from libusb upon completion of an IN transfer.
 */
void UsbDevice::handleRx(libusb_transfer *t)
{
    USB_TRACE(("USB: RX status %d\n", t->status));

    if (t->status == LIBUSB_TRANSFER_COMPLETED) {

        Packet pkt;
        pkt.len = t->actual_length;
        pkt.buf = (uint8_t*)malloc(pkt.len);
        memcpy(pkt.buf, t->buffer, pkt.len);
        mBufferedINPackets.push_back(pkt);

        // re-submit
        // XXX: should we be re-submitting in other statuses as well?
        int r = libusb_submit_transfer(t);
        if (r < 0)
            removeTransfer(mInEndpoint.pendingTransfers, t);

    } else {

        /*
         * Unless we cancelled it ourselves, open another request to replace it.
         */
        if (t->status != LIBUSB_TRANSFER_CANCELLED)
            submitINTransfer();

        removeTransfer(mInEndpoint.pendingTransfers, t);
    }
}

/*
 * Called back from libusb on completion of an OUT transfer.
 */
void UsbDevice::handleTx(libusb_transfer *t)
{
    USB_TRACE(("USB: TX status %d\n", t->status));

    removeTransfer(mOutEndpoint.pendingTransfers, t);

#if 0
    libusb_transfer_status status = t->status;
    if (status == LIBUSB_TRANSFER_COMPLETED) {
        int transferLen = t->actual_length;
        emit bytesWritten(transferLen);
    }
#endif
}

/*
 * A transfer has completed. Free its resources, and remove it from our queue.
 * If both endpoints queues are empty, it's time to close up shop.
 */
void UsbDevice::removeTransfer(std::list<libusb_transfer*> &list, libusb_transfer *t)
{
    assert(!list.empty());
    list.remove(t);

    free(t->buffer);
    libusb_free_transfer(t);

    if (mInEndpoint.pendingTransfers.empty() &&
        mOutEndpoint.pendingTransfers.empty())
    {
        libusb_close(mHandle);
        mHandle = 0;
    }
}

/*
 * Static callback handlers.
 */

void UsbDevice::onRxComplete(libusb_transfer *t)
{
    UsbDevice *dev = static_cast<UsbDevice *>(t->user_data);
    dev->handleRx(t);
}

void UsbDevice::onTxComplete(libusb_transfer *t)
{
    UsbDevice *dev = static_cast<UsbDevice *>(t->user_data);
    dev->handleTx(t);
}
