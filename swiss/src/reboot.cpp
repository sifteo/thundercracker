#include "reboot.h"
#include "basedevice.h"
#include "macros.h"
#include "usbprotocol.h"
#include "swisserror.h"

#include <stdio.h>


int Reboot::run(int argc, char **argv, IODevice &_dev)
{
    if (argc != 1) {
        fprintf(stderr, "incorrect args\n");
        return EINVAL;
    }
    
    Reboot cmd(_dev);
    return cmd.requestReboot();
}

Reboot::Reboot(IODevice &_dev) : dev(_dev) {}

int Reboot::requestReboot()
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return ENODEV;
    }

    BaseDevice base(dev);
    if (!base.requestReboot()) {
        return EIO;
    }

    return EOK;
}
