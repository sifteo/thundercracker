#include "usbhardware.h"
#include "stm32f10xotg.h"

using namespace Usb;

static Stm32f10xOtg stm32usb;

void UsbHardware::init()
{
    stm32usb.init();
}

void UsbHardware::setAddress(uint8_t addr)
{
    stm32usb.setAddress(addr);
}

void UsbHardware::epSetup(uint8_t addr, uint8_t type, uint16_t max_size, EpCallback cb)
{
    stm32usb.epSetup(addr, type, max_size, cb);
}

void UsbHardware::epReset()
{

}

void UsbHardware::epStallSet(uint8_t addr, uint8_t stall)
{

}

void UsbHardware::epNakSet(uint8_t addr, uint8_t nak)
{

}

bool UsbHardware::epIsStalled(uint8_t addr)
{
    return stm32usb.epIsStalled(addr);
}

uint16_t UsbHardware::epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    return stm32usb.epWritePacket(addr, buf, len);
}

uint16_t UsbHardware::epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    return stm32usb.epReadPacket(addr, buf, len);
}

void UsbHardware::poll()
{

}

void UsbHardware::disconnect(bool disconnected)
{

}

