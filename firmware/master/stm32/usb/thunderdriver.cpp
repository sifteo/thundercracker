#include "thunderdriver.h"

ThunderDriver::ThunderDriver()
{
}

void ThunderDriver::handleReset()
{

}

void ThunderDriver::handleSuspend()
{

}

void ThunderDriver::handleResume()
{

}

void ThunderDriver::handleStartOfFrame()
{

}

void ThunderDriver::onConfigComplete(uint16_t wValue)
{

}

int ThunderDriver::controlRequest(Usb::SetupData *req, uint8_t **buf, uint16_t *len)
{
    return 0;
}

void ThunderDriver::inEndpointCallback(uint8_t ep, Usb::Transaction txn)
{

}

void ThunderDriver::outEndpointCallback(uint8_t ep, Usb::Transaction txn)
{

}

