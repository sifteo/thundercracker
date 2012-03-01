/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef USBOTGDEFINES_H
#define USBOTGDEFINES_H

namespace UsbOtg {

static const unsigned MaxEp0Size = 64;
static const unsigned MaxInterfaceCount = 1;            // configurable - ifdef this?
static const unsigned MaxConfigurationCount = 1;        // configurable - ifdef this?
static const unsigned MaxStringDescriptionSize = 200;   // configurable - ifdef this?

static inline uint16_t SWAPBYTE(void *addr) {
    return (((uint16_t)(*((uint8_t *)(addr)))) +     (((uint16_t)(*(((uint8_t *)(addr)) + 1))) << 8));
}

static inline uint8_t LOBYTE(uint16_t n) {
    return n & 0xFF;
}

static inline uint8_t HIBYTE(uint16_t n) {
    return (n & 0xFF00) >> 8;
}

enum Mode {
    OtgMode,
    HostMode = (1 << 29),
    DeviceMode = (1 << 30)
};

enum EndpointType {
    Control = 0,
    Isochronous = 1,
    Bulk = 2,
    Interrupt = 3,
    Mask = 4
};

enum DeviceStatus {
    Default = 1,
    Addressed = 2,
    Configured = 3,
    Suspended = 4
};

enum DeviceStatusEnumSpeed {
    HighSpeedPhy30_or_60Mhz = 0,
    FullSpeedPhy30_or_60Mhz = 1,
    LowSpeedPhy6Mhz         = 2,
    FullSpeedPhy48Mhz       = 3
};

enum EpState {
    EpIdle,
    EpSetup,
    EpDataIn,
    EpDataOut,
    EpStatusIn,
    EpStatusOut,
    EpStall
};

enum EndpointDirection {
    In,
    Out
};

enum RequestType {
    RequestGetStatus        = 0x00,
    RequestClearFeature     = 0x01,
    RequestSetFeature       = 0x03,
    RequestSetAddress       = 0x05,
    RequestGetDescriptor    = 0x06,
    RequestSetDescriptor    = 0x07,
    RequestGetConfiguration = 0x08,
    RequestSetConfiguration = 0x09,
    RequestGetInterface     = 0x0A,
    RequestSetInterface     = 0x0B,
    RequestSyncFrame        = 0x0C
};

enum DescriptorType {
    DescriptorTypeDevice = 1,
    DescriptorTypeConfiguration = 2,
    DescriptorTypeString = 3,
    DescriptorTypeInterface = 4,
    DescriptorTypeEndpoint = 5,
    DescriptorTypeDeviceQualifier = 6,
    DescriptorTypeOtherSpeedConfiguration = 7
};

enum RequestRecipient {
    ReqRecipientDevice = 0,
    ReqRecipientInterface = 1,
    ReqRecipientEndpoint = 2,
    ReqRecipientMask = 3
};

enum DescriptorIndex {
    IndexLangIdString       = 0x00,
    IndexManufacturerString = 0x01,
    IndexProductString      = 0x02,
    IndexSerialNumString    = 0x03,
    IndexConfigString       = 0x04,
    IndexInterfaceString    = 0x05
};

enum Feature {
    FeatureEpHalt       = 0,
    FeatureRemoteWakeup = 1,
    FeatureTestMode     = 2
};

enum DeviceConfigFrameInterval {
    FrameInterval80 = 0,
    FrameInterval85 = 1,
    FrameInterval90 = 2,
    FrameInterval95 = 3
};

typedef struct Endpoint_t {
    uint8_t        num;
//    uint8_t        is_in;
    EndpointDirection direction;
    uint8_t        is_stall;
    EndpointType    type;
    uint8_t        data_pid_start;
    uint8_t        even_odd_frame;
    uint16_t       tx_fifo_num;
    uint32_t       maxpacket;
    // transaction level variables
    uint8_t        *xfer_buff;
    uint32_t       xfer_len;
    uint32_t       xfer_count;
    // Transfer level variables
    uint32_t       rem_data_len;
    uint32_t       total_data_len;
    uint32_t       ctl_data_len;
} Endpoint;

typedef struct SetupRequest_t {
    uint8_t   bmRequest;
    uint8_t   bRequest;
    uint16_t  wValue;
    uint16_t  wIndex;
    uint16_t  wLength;
} SetupRequest;

class DeviceClassInterface {
public:
    uint8_t  init(void *pdev, uint8_t cfgidx);
    uint8_t  deinit(void *pdev, uint8_t cfgidx);
    // Control Endpoints
    uint8_t  setup(void *pdev, SetupRequest *req);
    uint8_t  ep0txSent(void *pdev);
    uint8_t  ep0rxReady(void *pdev);
    // Class Specific Endpoints
    uint8_t  dataIn(void *pdev, uint8_t epnum);
    uint8_t  dataOut(void *pdev, uint8_t epnum);
    uint8_t  startOfFrame(void *pdev);
    uint8_t  isoINIncomplete(void *pdev);
    uint8_t  isoOUTIncomplete(void *pdev);
    uint8_t* getConfigDescriptor(uint8_t speed , uint16_t *length);

#ifdef USB_SUPPORT_USER_STRING_DESC
    uint8_t  *(*GetUsrStrDescriptor)( uint8_t speed ,uint8_t index,  uint16_t *length);
#endif
};

typedef struct UserCallbacks_t
{
  void (*Init)(void);
  void (*DeviceReset)(uint8_t speed);
  void (*DeviceConfigured)(void);
  void (*DeviceSuspended)(void);
  void (*DeviceResumed)(void);

  void (*DeviceConnected)(void);
  void (*DeviceDisconnected)(void);
} UserCallbacks;

} // namespace UsbOtg

#endif // USBOTGDEFINES_H
