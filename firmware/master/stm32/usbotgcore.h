/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef USB_OTG_CORE_H
#define USB_OTG_CORE_H

#include "hardware.h"
#include "gpio.h"
#include "usbotgdefines.h"

class UsbOtgCore
{
public:
    static UsbOtgCore instance;

    static const uint8_t    HOST_CHANNELS = 8;
    static const uint8_t    NUM_ENDPOINTS = 4;
    static const uint16_t   FIFO_SIZE = 320;
    static const uint8_t    MAX_PACKET_SIZE = 64;

    UsbOtgCore(GPIOPin _sof,
           GPIOPin _vbus,
           GPIOPin _dm,
           GPIOPin _dp) :
        sof(_sof), vbus(_vbus), dm(_dm), dp(_dp)
    {}
    void init();
    void reset();
    void stop();

    void enableGlobalIsr() {
        USBOTG.global.GINTMSK |= (1 << 12);
    }

    void disableGlobalIsr() {
        USBOTG.global.GINTMSK &= ~(1 << 12);
    }

    void enableCommonInterrupts();
    void enableDeviceInterrupts();
    void setMode(UsbOtg::Mode mode);

    void writePacket(uint8_t *src, uint8_t ch_ep_num, uint16_t len);
    void* readPacket(uint8_t *dest, uint16_t len);

    void initDevMode();
//    void initHostMode();    // if we ever want this

    void flushTxFifo(uint8_t num);
    void flushRxFifo();

    void setEndpointStall(UsbOtg::Endpoint *ep);
    void clearEndpointStall(UsbOtg::Endpoint *ep);

    void ep0StartXfer(UsbOtg::Endpoint *ep);
    void activateEp0();

    void isr();

    uint8_t speed() const { return _speed; }

private:
    GPIOPin sof;
    GPIOPin vbus;
    GPIOPin dm;
    GPIOPin dp;

    uint8_t       _speed;
    uint8_t       sofOutput;
    uint8_t       lowPower;
};

#endif // USB_OTG_CORE_H
