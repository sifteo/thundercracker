#include "usb/usbdriver.h"
#include "usb/thunderdriver.h"

static ThunderDriver driver;    // specify desired driver here

void UsbDriver::handleReset()
{
    driver.handleReset();
}

void UsbDriver::handleSuspend()
{
    driver.handleSuspend();
}

void UsbDriver::handleResume()
{
    driver.handleResume();
}

void UsbDriver::handleStartOfFrame()
{
    driver.handleStartOfFrame();
}

void UsbDriver::setConfig(uint16_t wValue)
{
    driver.setConfig(wValue);
}

int UsbDriver::controlRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len)
{
    return driver.controlRequest(req, buf, len);
}

void UsbDriver::inEndpointCallback(uint8_t ep, Usb::Transaction txn)
{
    driver.inEndpointCallback(ep, txn);
}

void UsbDriver::outEndpointCallback(uint8_t ep, Usb::Transaction txn)
{
    driver.outEndpointCallback(ep, txn);
}
