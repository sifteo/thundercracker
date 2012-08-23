#ifndef BACKUP_H
#define BACKUP_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

class Backup
{
public:
    Backup(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    bool backup(const char *path);

private:
    IODevice &dev;

    unsigned requestProgress;
    unsigned replyProgress;

    static const unsigned DEVICE_SIZE = 16*1024*1024;
    static const unsigned PAGE_SIZE = 256;
    static const unsigned BLOCK_SIZE = 64*1024;

    bool writeFileHeader(FILE *f);
    bool writeFlashContents(FILE *f);

    bool sendRequest();
    bool writeReply(FILE *f);
};

#endif // BACKUP_H
