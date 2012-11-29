#include "metadata.h"
#include "basedevice.h"
#include "usbvolumemanager.h"

Metadata::Metadata(IODevice &_dev) :
    dev(_dev)
{
}

std::string Metadata::getString(unsigned volBlockCode, unsigned key)
{
    USBProtocolMsg buf;
    BaseDevice base(dev);
    if (!base.getMetadata(buf, volBlockCode, key)) {
        return "(none)";
    }

    return std::string(buf.castPayload<char>());
}

int Metadata::getBytes(unsigned volBlockCode, unsigned key, uint8_t *buffer, unsigned len)
{
    USBProtocolMsg buf;
    BaseDevice base(dev);
    if (!base.getMetadata(buf, volBlockCode, key)) {
        return -1;
    }

    unsigned chunk = std::min(len, buf.len);
    memcpy(buffer, buf.castPayload<uint8_t>(), chunk);

    return chunk;
}
