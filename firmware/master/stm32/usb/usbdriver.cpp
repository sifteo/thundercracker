#include "usbdriver.h"
#include "stm32f10xotg.h"

using namespace Usb;

static Stm32f10xOtg stm32usb;

void UsbDriver::init()
{
    stm32usb.init();
}

void UsbDriver::setAddress(uint8_t addr)
{
    stm32usb.setAddress(addr);
}

void UsbDriver::epSetup(uint8_t addr, uint8_t type, uint16_t max_size, epCallback cb)
{
    stm32usb.epSetup(addr, type, max_size, cb);
}

void UsbDriver::epReset()
{

}

void UsbDriver::epStallSet(uint8_t addr, uint8_t stall)
{

}

void UsbDriver::epNakSet(uint8_t addr, uint8_t nak)
{

}

bool UsbDriver::epIsStalled(uint8_t addr)
{
    return stm32usb.epIsStalled(addr);
}

uint16_t UsbDriver::epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    return stm32usb.epWritePacket(addr, buf, len);
}

uint16_t UsbDriver::epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    return stm32usb.epReadPacket(addr, buf, len);
}

void UsbDriver::poll()
{

}

void UsbDriver::disconnect(bool disconnected)
{

}

