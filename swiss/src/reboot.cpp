#include "reboot.h"
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

    USBProtocolMsg m(USBProtocol::FactoryTest);
    m.append(12);   // reboot request command
    dev.writePacket(m.bytes, m.len);
    return true;
}
