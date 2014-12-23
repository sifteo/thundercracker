/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "listen.h"
#include "libusb.h"
#include "swisserror.h"

#include <stdio.h>

sig_atomic_t Listen::interruptRequested;

int Listen::run(int argc, char **argv, IODevice &_dev)
{
    if (argc < 2) {
        fprintf(stderr, "listen: need at least 2 args\n");
        return EINVAL;
    }

    if (signal(SIGINT, onSignal) == SIG_ERR) {
        fputs("listen: An error occurred while setting a signal handler.\n", stderr);
        return 1;
    }

    // fixed position arg for elfpath
    char *elfpath = argv[1];

    // optional out file, defaults to stdout
    char *outpath = NULL;
    bool flush = false;
    for (int i = 2; i < argc; ++i) {

        if (strcmp(argv[i], "--flush-logs") == 0) {
            flush = true;
        }

        if (i + 1 < argc && !strcmp(argv[i], "--fout")) {
            outpath = argv[i + 1];
            i++;
        }

    }

    Listen listener(_dev);
    return listener.listen(elfpath, outpath, flush);
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

int Listen::listen(const char *elfpath, const char * outpath, bool flushLogs)
{
    if (!dev.open(IODevice::SIFTEO_VID, IODevice::BASE_PID)) {
        return ENODEV;
    }

    if (!dbgInfo.init(elfpath)) {
        fprintf(stderr, "listen: couldn't initialize elf: %s\n", elfpath);
        return ENOENT;
    }

    FILE *fout;
    if (!getFileOrStdout(&fout, outpath)) {
        return ENOENT;
    }

    logDecoder.init(flushLogs);

    while (!interruptRequested) {

        if (dev.numPendingINPackets() == 0) {
            if (dev.processEvents(10) < 0) {
                fprintf(stderr, "listen: error in processEvents()\n");
                return EIO;
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
    return EOK;
}

bool Listen::writeRecord(FILE *f, const USBProtocolMsg & m)
{
    // mask off high 4 bits which specify the USBProtocolMsg subsystem
    SvmLogTag tag(m.header & 0xfffffff);
    size_t sz = logDecoder.decode(f, dbgInfo, tag, reinterpret_cast<const uint32_t*>(&m.payload[0]));
    if (sz != m.payloadLen() - SvmDebugPipe::LOG_BUFFER_WORDS) {
        fprintf(stderr, "unexpected decode len: want %d got %d\n", m.payloadLen() - SvmDebugPipe::LOG_BUFFER_WORDS, int(sz));
    }

    return true;
}
