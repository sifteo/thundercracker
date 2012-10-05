#include "metadata.h"
#include "usbvolumemanager.h"

Metadata::Metadata(IODevice &_dev) :
    dev(_dev)
{
}

std::string Metadata::getString(unsigned volBlockCode, unsigned key)
{
    USBProtocolMsg buf;
    if (!get(buf, volBlockCode, key)) {
        return "(none)";
    }

    return std::string(buf.castPayload<char>(), buf.payloadLen());
}

int Metadata::getBytes(unsigned volBlockCode, unsigned key, uint8_t *buffer, unsigned len)
{
    USBProtocolMsg buf;
    if (!get(buf, volBlockCode, key)) {
        return -1;
    }

    unsigned chunk = std::min(len, buf.len);
    memcpy(buffer, buf.castPayload<uint8_t>(), chunk);

    return chunk;
}

bool Metadata::get(USBProtocolMsg &buffer, unsigned volBlockCode, unsigned key)
{
    buffer.init(USBProtocol::Installer);
    buffer.header |= UsbVolumeManager::VolumeMetadata;
    UsbVolumeManager::VolumeMetadataRequest *req = buffer.zeroCopyAppend<UsbVolumeManager::VolumeMetadataRequest>();

    req->volume = volBlockCode;
    req->key = key;

    if (!dev.writeAndWaitForReply(buffer)) {
        return false;
    }

    return buffer.payloadLen() != 0;
}

