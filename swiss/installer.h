#ifndef INSTALLER_H
#define INSTALLER_H

#include "iodevice.h"

class Installer
{
public:
    Installer(IODevice &_dev);

    // entry point for the 'profile' command
    static int run(int argc, char **argv, IODevice &_dev);

    // in sync with firmware/master/common/usbvolumemanager.h
    enum Command {
        WriteHeader,
        WritePayload,
        WriteCommit
    };

    bool install(const char *path, int vid, int pid);

private:
    bool sendHeader(uint32_t filesz);
    bool commit();

    IODevice &dev;
};

#endif // INSTALLER_H
