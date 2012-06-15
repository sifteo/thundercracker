#ifndef PROFILER_H
#define PROFILER_H

#include "iodevice.h"

class Profiler
{
public:
    Profiler(IODevice &_dev);

    // entry point for the 'profile' command
    static int run(int argc, char **argv, IODevice &_dev);

    bool profile(const char *elfPath, const char *outPath);

private:
    IODevice &dev;
};

#endif // PROFILER_H
