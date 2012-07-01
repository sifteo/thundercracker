#ifndef DELETE_H
#define DELETE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

class Delete
{
public:
    Delete(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool deleteEverything();

private:
    IODevice &dev;
};

#endif // INSTALLER_H
