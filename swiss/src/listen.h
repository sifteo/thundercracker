#ifndef LISTEN_H
#define LISTEN_H

#include "iodevice.h"
#include "elfdebuginfo.h"
#include "logdecoder.h"

#include <signal.h>

class Listen
{
public:
    static int run(int argc, char **argv, IODevice &_dev);

private:
    Listen(IODevice &_dev);

    int listen(const char *elfpath, const char *outpath, bool flushLogs=false);
    bool writeRecord(FILE *f, const USBProtocolMsg &m);

    IODevice &dev;
    ELFDebugInfo dbgInfo;
    LogDecoder logDecoder;


    static bool getFileOrStdout(FILE **f, const char *path);
    static void onSignal(int sig);
    static sig_atomic_t interruptRequested;
};

#endif // LISTEN_H
