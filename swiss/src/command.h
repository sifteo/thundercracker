#ifndef COMMAND_H
#define COMMAND_H

#include "iodevice.h"

typedef int (*CommandRun)(int argc, char **argv, IODevice &dev);

struct Command {
    const char *name;
    const char *description;
    const char *usage;
    CommandRun run;
};

#endif // COMMAND_H
