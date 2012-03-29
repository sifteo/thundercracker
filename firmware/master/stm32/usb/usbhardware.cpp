#include "usbhardware.h"
#include "stm32f10xotg.h"

using namespace Usb;

static Stm32f10xOtg hw;   // select desired hardware here

void UsbHardware::init()
{
    hw.init();
}

void UsbHardware::setAddress(uint8_t addr)
{
    hw.setAddress(addr);
}

void UsbHardware::epSetup(uint8_t addr, uint8_t type, uint16_t max_size)
{
    hw.epSetup(addr, type, max_size);
}

void UsbHardware::epReset()
{
    hw.epReset();
}

void UsbHardware::epSetStalled(uint8_t addr, bool stalled)
{
    hw.epSetStall(addr, stalled);
}

bool UsbHardware::epTxInProgress(uint8_t addr)
{
    return hw.epTxInProgress(addr);
}

void UsbHardware::epSetNak(uint8_t addr, bool nak)
{
    hw.epSetNak(addr, nak);
}

bool UsbHardware::epIsStalled(uint8_t addr)
{
    return hw.epIsStalled(addr);
}

uint16_t UsbHardware::epTxWordsAvailable(uint8_t addr)
{
    return hw.epTxWordsAvailable(addr);
}

uint16_t UsbHardware::epWritePacket(uint8_t addr, const void *buf, uint16_t len)
{
    return hw.epWritePacket(addr, buf, len);
}

uint16_t UsbHardware::epReadPacket(uint8_t addr, void *buf, uint16_t len)
{
    return hw.epReadPacket(addr, buf, len);
}

void UsbHardware::setDisconnected(bool disconnected)
{
    hw.setDisconnected(disconnected);
}

void UsbHardware::isr()
{
    hw.isr();
}

