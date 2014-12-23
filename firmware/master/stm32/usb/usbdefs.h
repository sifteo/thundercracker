/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

#ifndef USB_DEFS_H
#define USB_DEFS_H

#include <stdint.h>
#include "macros.h"

namespace Usb {

static ALWAYS_INLINE uint8_t lowByte(uint16_t x) {
    return x & 0xff;
}

static ALWAYS_INLINE uint8_t highByte(uint16_t x) {
    return (x >> 8) & 0xff;
}

#define USB_DESC_WORD(w)                                                    \
  (uint8_t)((w) & 255),                                                     \
  (uint8_t)(((w) >> 8) & 255)

static const unsigned REQ_DIR_MASK  = (1 << 7); // Bit 7=1: Direction bit
static const unsigned REQ_DIR_IN    = (1 << 7); // Bit 7=1: Device-to-host
static const unsigned REQ_DIR_OUT   = (0 << 7); // Bit 7=0: Host-to-device

static ALWAYS_INLINE bool reqIsIN(uint8_t type) {
    return (type & REQ_DIR_MASK) != 0;
}

static ALWAYS_INLINE bool reqIsOUT(uint8_t type) {
    return (type & REQ_DIR_MASK) == 0;
}

static ALWAYS_INLINE bool isInEp(uint8_t addr) {
    return (addr & 0x80) != 0;
}

typedef void (*EpCallback)(uint8_t ep);
typedef void (*Callback)();

enum Transaction {
    TransactionIn,
    TransactionOut,
    TransactionSetup
};

// bmRequestType bit definitions
enum RequestType {
    ReqTypeDevice           = 0x00,
    ReqTypeInterface        = 0x01,
    ReqTypeEndpoint         = 0x02,
    ReqTypeClass            = 0x20,
    ReqTypeVendor           = 0x40,
    ReqTypeMask             = 0x60,
    ReqTypeDirection        = 0x80,
    ReqTypeRecipientMask    = 0x1f
};

enum RequestVal {
    RequestGetStatus        = 0x0,
    RequestClearFeature     = 0x1,
    RequestSetFeature       = 0x3,
    RequestSetAddress       = 0x5,
    RequestGetDescriptor    = 0x6,
    RequestSetDescriptor    = 0x7,
    RequestGetConfiguration = 0x8,
    RequestSetConfiguration = 0x9,
    RequestGetInterface     = 0xa,
    RequestSetInterface     = 0xb,
    RequestSyncFrame        = 0xc
};

enum Feature {
    FeatureEndpointHalt = 0,
    FeatureDeviceRemoteWakeup = 1,
    FeatureTestMode = 2
};

enum EndpointType {
    Control     = 0,
    Isochronous = 1,
    Bulk        = 2,
    Interrupt   = 3,
    Mask        = 4
};

enum EndpointAttr {
    EpAttrControl = 0x00,
    EpAttrIsochronous = 0x01,
    EpAttrBulk = 0x02,
    EpAttrInterrupt = 0x03
};

enum DescriptorType {
    DescriptorDevice                    = 1,
    DescriptorConfiguration             = 2,
    DescriptorString                    = 3,
    DescriptorInterface                 = 4,
    DescriptorEndpoint                  = 5,
    DescriptorDeviceQualifier           = 6,
    DescriptorOtherSpeedConfiguration   = 7
};

enum DescriptorIndex {
    IndexLangIdString       = 0,
    IndexManufacturerString = 1,
    IndexProductString      = 2,
    IndexSerialNumString    = 3,
    IndexConfigString       = 4,
    IndexInterfaceString    = 5
};


struct SetupData {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__((packed));


struct Request {
    uint8_t   bmRequestType;
    uint8_t   bRequest;
    uint16_t  wValue;
    uint16_t  wIndex;
    uint16_t  wLength;
} __attribute__((packed));


struct DeviceDescriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} __attribute__((packed));


struct ConfigDescriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttibutes;
    uint8_t  MaxPower;
} __attribute__((packed));


struct InterfaceDescriptor {
    uint8_t bLength;
    uint8_t bDescriptorType;
    uint8_t bInterfaceNumber;
    uint8_t bAlternateSetting;
    uint8_t bNumEndpoints;
    uint8_t bInterfaceClass;
    uint8_t bInterfaceSubClass;
    uint8_t bInterfaceProtocol;
    uint8_t iInterface;
} __attribute__((packed));


struct EndpointDescriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} __attribute__((packed));


struct StringDescriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wstring[1];
} __attribute__((packed));


struct LanguageDescriptor {
    uint8_t  bLength;
    uint8_t  bDescriptorType;
    uint16_t wlangid[1];
} __attribute__((packed));

/************************************************************************
 *
 * Windows specific descriptors
 * http://msdn.microsoft.com/en-gb/library/windows/hardware/hh450799.aspx
 *
 ************************************************************************/

struct WinUsbCompatIdHeaderDescriptor {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint8_t  bCount;
    uint8_t  reserved0[7];
} __attribute__((packed));


struct WinUsbFunctionSectionDescriptor {
    uint8_t  bFirstInterfaceNumber;
    uint8_t  reserved1;
    char     compatibleID[8];
    char     subCompatibleID[8];
    uint8_t  reserved2[6];
} __attribute__((packed));


struct WinUsbExtPropHeaderDescriptor {
    uint32_t dwLength;
    uint16_t bcdVersion;
    uint16_t wIndex;
    uint16_t wCount;
} __attribute__((packed));


template<unsigned namesz, unsigned valsz>
struct WinUsbCustomPropDescriptor {
    uint32_t dwSize;
    uint32_t dwPropertyDataType;
    uint16_t wPropertyNameLength;
    uint16_t bPropertyName[namesz];
    uint32_t wPropertyValueLength;
    uint16_t bPropertyValue[valsz];
} __attribute__((packed));

} // namespace Usb

#endif // USB_DEFS_H
