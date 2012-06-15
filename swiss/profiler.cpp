#include "profiler.h"
#include "elfdebuginfo.h"
#include "usbprotocol.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

int Profiler::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 3) {
        fprintf(stderr, "not enough args\n");
        return 1;
    }

    Profiler profiler(_dev);
    bool success = profiler.profile(argv[1], argv[2]);
    return success ? 0 : 1;
}

Profiler::Profiler(IODevice &_dev) :
    dev(_dev)
{
}

bool Profiler::profile(const char *elfPath, const char *outPath)
{
    ELFDebugInfo dbgInfo;
    if (!dbgInfo.init(elfPath)) {
        fprintf(stderr, "couldn't open %s: %s\n", elfPath, strerror(errno));
        return false;
    }

    FILE *fout;
    if (!strcmp(outPath, "stderr")) {
        fout = stderr;
    } else if (!strcmp(outPath, "stdout")) {
        fout = stderr;
    }
    else {
        if (!(fout = fopen(outPath, "rb"))) {
            fprintf(stderr, "could not open %s: %s\n", outPath, strerror(errno));
            return false;
        }
    }

    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    USBProtocolMsg m(USBProtocol::Profiler);
    m.payload[0] = 0;
    m.payload[1] = 1;
    m.len += 2;
    dev.writePacket(m.bytes, m.len);

    // XXX: best way to break out?
    for (;;) {

        while (!dev.numPendingINPackets())
            dev.processEvents();

        m.len = dev.readPacket(m.bytes, m.MAX_LEN);
        if (!m.len) {
            fprintf(stderr, "zlp, continue\n");
            fflush(stderr);
            continue;
        }

        unsigned numSamples = 1; //m.payload[0];
        const uint32_t *address = reinterpret_cast<uint32_t*>(m.payload); // + 1);

        for (unsigned i = 0; i < numSamples; ++i) {
            // XXX: this works but is actually too slow to keep up with USB data!
            // need to mmap file access in ELFDebugInfo.
            std::string s = dbgInfo.formatAddress(*address++);
            fprintf(fout, "addr! %s (0x%x)\n", s.c_str(), address);
        }
    }

    return true;
}
