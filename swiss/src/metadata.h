#ifndef METADATA_H
#define METADATA_H

#include "iodevice.h"
#include "usbprotocol.h"
#include <string>

class Metadata
{
public:
    Metadata(IODevice &_dev);

    std::string getString(unsigned volBlockCode, unsigned key);
    int getBytes(unsigned volBlockCode, unsigned key, uint8_t *buffer, unsigned len);

private:
    IODevice &dev;
};

#endif // METADATA_H
