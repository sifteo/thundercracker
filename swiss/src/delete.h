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
    bool deleteSysLFS();
    bool deleteVolume(unsigned code);

private:
    IODevice &dev;

    static bool parseVolumeCode(const char *str, unsigned &code);
};

#endif // INSTALLER_H
