#ifndef PROFILER_H
#define PROFILER_H

#include "iodevice.h"

class Profiler
{
public:
    Profiler();

    // entry point for the 'profile' command
    static int run(int argc, char **argv, IODevice &dev);
};

#endif // PROFILER_H
