#ifndef DELETE_H
#define DELETE_H

#include "iodevice.h"
#include "usbvolumemanager.h"

class Delete
{
public:
    Delete(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    int deleteEverything();
    int deleteReformat();
    int deleteSysLFS();
    int deleteVolume(unsigned code);

private:
    IODevice &dev;
    bool getVolumeCode(const char *s, unsigned &volumeCode);
};

#endif // INSTALLER_H
