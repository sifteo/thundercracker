#ifndef INSTALLER_H
#define INSTALLER_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

class Installer
{
public:
    Installer(IODevice &_dev);

    // entry point for the 'profile' command
    static int run(int argc, char **argv, IODevice &_dev);

    bool install(const char *path, int vid, int pid);

private:
    bool sendHeader(uint32_t filesz, const std::string &pkg);
    bool getPackageMetadata(const char *path, std::string &pkg, std::string &version);
    bool sendFileContents(FILE *f, uint32_t filesz);
    bool commit();

    IODevice &dev;
};

#endif // INSTALLER_H
