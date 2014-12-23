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

#include "usbdevice.h"
#include "libusb.h"
#include <assert.h>

#if 0
#define USB_TRACE(_x)   printf _x
#else
#define USB_TRACE(_x)
#endif


UsbDevice::UsbDevice() :
    mInterface(-1),
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

    bool foundAnyDevice = false;
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
        foundAnyDevice = true;

        USB_TRACE(("USB: Starting libusb_open()\n"));

        libusb_device_handle *handle;
        r = libusb_open(dev, &handle);
        if (r < 0) {
            if (r == LIBUSB_ERROR_ACCESS)
                fprintf(stderr, "insufficient permissions to open device\n");
            else
                fprintf(stderr, "error opening device: %s\n", libusb_error_name(r));
            continue;
        }

        libusb_config_descriptor *cfg;
        r = libusb_get_config_descriptor(dev, 0, &cfg);
        if (r < 0) {
            fprintf(stderr, "error communicating with device: %s\n", libusb_error_name(r));
        } else {
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

    if (!isOpen()) {
        if (!foundAnyDevice)
            fprintf(stderr, "device is not attached\n");
        return false;
    }

    USB_TRACE(("USB: Claiming interface\n"));
    int r = libusb_claim_interface(mHandle, interface);
    if (r < 0) {
        fprintf(stderr, "error claiming exclusive access to device: %s\n", libusb_error_name(r));
        return false;
    }

    mInterface = interface;

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

    int r = libusb_release_interface(mHandle, mInterface);
    if (r < 0) {
        fprintf(stderr, "libusb_release_interface error: %s\n", libusb_error_name(r));
    }

    libusb_close(mHandle);
    mHandle = 0;

    releaseTransfers(mInEndpoint);
    releaseTransfers(mOutEndpoint);
}

void UsbDevice::releaseTransfers(Endpoint &ep)
{
    for (std::list<libusb_transfer*>::iterator i = ep.pendingTransfers.begin();
         i != ep.pendingTransfers.end();)
    {
        libusb_transfer *t = *i;
        free(t->buffer);
        libusb_free_transfer(t);
        i = ep.pendingTransfers.erase(i);
    }
}

bool UsbDevice::isOpen() const
{
    return mHandle != 0;
}

int UsbDevice::readPacket(uint8_t *buf, unsigned maxlen, unsigned & rxlen)
{
    /*
     * Dequeue a packet that has already been received.
     */

    assert(!mBufferedINPackets.empty());

    RxPacket pkt = mBufferedINPackets.front();
    rxlen = MIN(maxlen, pkt.len);
    memcpy(buf, pkt.buf, rxlen);

    delete pkt.buf;
    mBufferedINPackets.pop_front();

    USB_TRACE(("USB: Read %d bytes, %02x%02x%02x%02x %02x%02x%02x%02x ...\n",
        rxlen, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]));

    return pkt.status;
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
        return r;
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

    mBufferedINPackets.push_back(RxPacket(t));

    // re-submit
    int r = libusb_submit_transfer(t);
    if (r < 0) {
        fprintf(stderr, "failed to re-submit: %s\n", libusb_error_name(r));
        removeTransfer(mInEndpoint, t);
    }
}

/*
 * Called back from libusb on completion of an OUT transfer.
 */
void UsbDevice::handleTx(libusb_transfer *t)
{
    USB_TRACE(("USB: TX status %d\n", t->status));

    removeTransfer(mOutEndpoint, t);

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
void UsbDevice::removeTransfer(Endpoint &ep, libusb_transfer *t)
{
    free(t->buffer);
    libusb_free_transfer(t);
    assert(!ep.pendingTransfers.empty());
    ep.pendingTransfers.remove(t);
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
