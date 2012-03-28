#include "usb/usbdriver.h"
#include "usb/usbhardware.h"
#include "usb.h"

#include "tasks.h"

using namespace Usb;

void UsbDriver::handleReset()
{
}

void UsbDriver::handleSuspend()
{
}

void UsbDriver::handleResume()
{
}

void UsbDriver::handleStartOfFrame()
{
}

/*
 * Our configuration has been set, we're now ready to enable the endpoints
 * for the selected configuration - we only ever have one for this device.
 */
void UsbDriver::onConfigComplete(uint16_t wValue)
{
    UsbHardware::epSetup(UsbDevice::InEpAddr, EpAttrBulk, 64);
    UsbHardware::epSetup(UsbDevice::OutEpAddr, EpAttrBulk, 64);
}

/*
 * We have no specific control requests that we need to handle for this device.
 */
int UsbDriver::controlRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len)
{
    return 0;
}

void UsbDriver::inEndpointCallback(uint8_t ep)
{
}

void UsbDriver::outEndpointCallback(uint8_t ep)
{
    Tasks::setPending(Tasks::UsbOUT, 0);
}
