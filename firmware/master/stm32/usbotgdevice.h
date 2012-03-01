/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef USBOTGDEVICE_H
#define USBOTGDEVICE_H

#include "usbotgcore.h"
#include "usbotgdefines.h"

/*
    Represent a USB OTG device (as opposed to host).
    Specific class types should be able to hook in
    here and have their lower level comm details handled.
*/
class UsbOtgDevice
{
public:
    static UsbOtgDevice instance;

    UsbOtgDevice() :
        usbcore(UsbOtgCore::instance),
        USBD_ep_status(0), USBD_default_cfg(0), USBD_cfg_status(0)
    {}
    void init();

    void openEndpoint(uint8_t ep_addr, uint16_t ep_mps, UsbOtg::EndpointType ep_type);
    void setEndpointStall(uint8_t epnum);
    void clearEndpointStall(uint8_t epnum);

    void isr(uint32_t deviceStatus);

    // control helpers
    void controlError(UsbOtg::SetupRequest &req);
    void controlSendStatus();
    void controlReceiveStatus();
    void controlPrepareRx(uint8_t *pbuf, uint16_t len);
    void controlContinueRx(uint8_t *pbuf, uint16_t len);
    void controlContinueTx(uint8_t *pbuf, uint16_t len);
    void controlTx(uint8_t *pbuf, uint16_t len);

    void endpointTx(uint8_t ep_addr, uint8_t *pbuf, uint32_t buf_len);

private:
    UsbOtgCore &usbcore;

    uint8_t                 deviceConfig;
    UsbOtg::EpState         ep0State;
    UsbOtg::DeviceStatus    deviceStatus;
    uint8_t                 address;
//    uint32_t       DevRemoteWakeup;
    UsbOtg::Endpoint        inEps[UsbOtgCore::NUM_ENDPOINTS];
    UsbOtg::Endpoint        outEps[UsbOtgCore::NUM_ENDPOINTS];
    uint8_t                 setup_packet[8 * 3];
//    USBD_Class_cb_TypeDef         *class_cb;
//    USBD_Usr_cb_TypeDef           *usr_cb;
//    USBD_DEVICE                   *usr_device;
//    uint8_t        *pConfig_descriptor;

    UsbOtg::DeviceClassInterface *deviceClass;  // specific USB class profile
    UsbOtg::UserCallbacks *userCallbacks;       // end user events

    // TODO - rename these
    uint32_t USBD_ep_status;
    uint32_t USBD_default_cfg;
    uint32_t USBD_cfg_status;
    uint8_t  USBD_StrDesc[UsbOtg::MaxStringDescriptionSize];

    // isr subroutines
    void handleOutEp_isr();
    void setupStage_isr();
    void handleInEp_isr();
    void dataInStage_isr(uint8_t epnum);
    void handleResume_isr();
    void handleUSBSuspend_isr();
    void dataOutStage_isr(uint8_t epnum);
    void handleUsbReset_isr();
    void handleEnumDone_isr();

    // requrest helpers
    void standardDeviceReq(UsbOtg::SetupRequest &req);
    void requestGetDescriptor(UsbOtg::SetupRequest &req);
    void requestSetAddress(UsbOtg::SetupRequest &req);
    void requestGetStatus(UsbOtg::SetupRequest &req);
    void requestSetConfig(UsbOtg::SetupRequest &req);
    void requestGetConfig(UsbOtg::SetupRequest &req);
    void setFeature(UsbOtg::SetupRequest &req);
    void clearFeature(UsbOtg::SetupRequest &req);
    void parseSetupRequest(UsbOtg::SetupRequest &req);
    void standardInterfaceReq(UsbOtg::SetupRequest &req);
    void standardEndpointReq(UsbOtg::SetupRequest &req);
};

#endif // USBOTGDEVICE_H
