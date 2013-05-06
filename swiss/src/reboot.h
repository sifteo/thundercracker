#ifndef REBOOT_H
#define REBOOT_H

#include "iodevice.h"
#include <stdio.h>

class Reboot
{
public:
    Reboot(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &dev);

private:
    IODevice &dev;

    int requestReboot();
};

#endif // REBOOT_H
