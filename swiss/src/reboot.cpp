#include "reboot.h"
#include "basedevice.h"
#include "macros.h"
#include "usbprotocol.h"

#include <stdio.h>


int Reboot::run(int argc, char **argv, IODevice &_dev)
{
    if (argc != 1) {
        fprintf(stderr, "incorrect args\n");
        return 1;
    }
    
    Reboot cmd(_dev);
    return !cmd.requestReboot();
}

Reboot::Reboot(IODevice &_dev) : dev(_dev) {}

bool Reboot::requestReboot()
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID))
        return false;

    BaseDevice base(dev);
    return base.requestReboot();
}
