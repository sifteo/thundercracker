#include "profiler.h"

#include <stdio.h>

Profiler::Profiler()
{
}

int Profiler::run(int argc, char **argv, IODevice &dev)
{
    fprintf(stderr, "Profiler run!\n");
    return 0;
}
