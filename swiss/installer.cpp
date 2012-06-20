#include "installer.h"
#include "usbprotocol.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

int Installer::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2) {
        fprintf(stderr, "not enough args\n");
        return 1;
    }

    Installer installer(_dev);

    bool success = installer.install(argv[1], IODevice::SIFTEO_VID, IODevice::BASE_PID);

    _dev.close();
    _dev.processEvents();

    return success ? 0 : 1;
}

Installer::Installer(IODevice &_dev) :
    dev(_dev)
{
}

bool Installer::sendHeader(uint32_t filesz)
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= WriteHeader;
    *reinterpret_cast<uint32_t*>(m.payload) = filesz;
    m.len += sizeof(filesz);

    dev.writePacket(m.bytes, m.len);
    dev.processEvents();
}

bool Installer::install(const char *path, int vid, int pid)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    if (!dev.open(vid, pid)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    fseek(f, 0L, SEEK_END);
    uint32_t filesz = ftell(f);
    fseek(f, 0L, SEEK_SET);
    sendHeader(filesz);

    for (volatile unsigned i = 0; i < 100000000; ++i)
        ;

    unsigned percent = 0;
    unsigned progress = 0;

    while (!feof(f)) {

        USBProtocolMsg m(USBProtocol::Installer);
        m.header |= WritePayload;
        m.len += fread(m.payload, 1,  m.bytesFree(), f);
        if (m.len <= 0)
            continue;

        dev.writePacket(m.bytes, m.len);
        while (dev.numPendingOUTPackets() > IODevice::MAX_OUTSTANDING_OUT_TRANSFERS)
            dev.processEvents();

        progress += m.len;
        const unsigned progressPercent = ((float)progress / (float)filesz) * 100;
        if (progressPercent != percent) {
            percent = progressPercent;
            fprintf(stderr, "progress: %d%%\n", percent);
            fflush(stderr);
        }
    }

    commit();

    fclose(f);

    while (dev.numPendingOUTPackets()) {
        dev.processEvents();
    }
}

/*
 * We're done sending payload data - tell the master to commit this app.
 */
bool Installer::commit()
{
    USBProtocolMsg m(USBProtocol::Installer);
    m.header |= WriteCommit;

    dev.writePacket(m.bytes, m.len);
    dev.processEvents();
}
