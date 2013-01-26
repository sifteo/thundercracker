#include "listen.h"
#include "libusb.h"

#include <stdio.h>
#include <errno.h>

sig_atomic_t Listen::interruptRequested;

int Listen::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2) {
        fprintf(stderr, "listen: need at least 2 args\n");
        return 1;
    }

    if (signal(SIGINT, onSignal) == SIG_ERR) {
        fputs("listen: An error occurred while setting a signal handler.\n", stderr);
        return 1;
    }

    // fixed position arg for elfpath
    char *elfpath = argv[1];

    // optional out file, defaults to stdout
    char *outpath = NULL;
    for (int i = 2; i < argc; ++i) {

        if (i + 1 < argc && !strcmp(argv[i], "--fout")) {
            outpath = argv[i + 1];
            i++;
        }
    }

    Listen listener(_dev);
    return listener.listen(elfpath, outpath) ? 0 : 1;
}

Listen::Listen(IODevice &_dev) :
    dev(_dev)
{

}

void Listen::onSignal(int sig) {
    if (sig == SIGINT) {
        interruptRequested = true;
    }
}

bool Listen::getFileOrStdout(FILE **f, const char *path)
{
    if (path) {

        *f = fopen(path, "w");
        if (*f == NULL) {
            fprintf(stderr, "can't open %s (%s)\n", path, strerror(errno));
            return false;
        }

    } else {
        *f = stdout;
    }

    return true;
}

bool Listen::listen(const char *elfpath, const char * outpath)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return false;
    }

    if (!dbgInfo.init(elfpath)) {
        fprintf(stderr, "listen: couldn't initialize elf: %s\n", elfpath);
        return false;
    }

    FILE *fout;
    if (!getFileOrStdout(&fout, outpath)) {
        return false;
    }

    logDecoder.init();

    while (!interruptRequested) {

        if (dev.numPendingINPackets() == 0) {
            if (dev.processEvents(10) < 0) {
                fprintf(stderr, "listen: error in processEvents()\n");
                return false;
            }

            // yield so we can check again for an interrupt request
            if (dev.numPendingINPackets() == 0) {
                continue;
            }
        }

        USBProtocolMsg m;
        int r = dev.readPacket(m.bytes, m.MAX_LEN, m.len);
        if (r != LIBUSB_TRANSFER_COMPLETED) {
            if (r == LIBUSB_TRANSFER_ERROR || r == LIBUSB_TRANSFER_OVERFLOW) {
                fprintf(stderr, "listen: io error: %d\n", r);
            }
            break;
        }

        if (m.subsystem() == USBProtocol::Logger) {
            if (!writeRecord(fout, m)) {
                break;
            }
        }
    }

    fclose(fout);
    return true;
}

bool Listen::writeRecord(FILE *f, const USBProtocolMsg & m)
{
    // mask off high 4 bits which specify the USBProtocolMsg subsystem
    SvmLogTag tag(m.header & 0xfffffff);
    size_t sz = logDecoder.decode(dbgInfo, tag, reinterpret_cast<const uint32_t*>(&m.payload[0]));
    if (sz != m.payloadLen() - SvmDebugPipe::LOG_BUFFER_WORDS) {
        fprintf(stderr, "unexpected decode len: want %d got %d\n", m.payloadLen() - SvmDebugPipe::LOG_BUFFER_WORDS, sz);
    }

    return true;
}
