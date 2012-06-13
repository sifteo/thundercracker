
#include "command.h"
#include "profiler.h"

#include <stdio.h>
#include <string.h>

static const Command commands[] = {
    {
        "profile",
        "capture profiling data from an app",
        "profile <opts>",
        Profiler::run
    }
};

static void usage()
{
    fprintf(stderr, "swiss - your Sifteo utility knife\n");
    const unsigned numCommands = sizeof(commands) / sizeof(commands[0]);
    for (unsigned i = 0; i < numCommands; ++i)
        fprintf(stderr, "    %s:    %s\n", commands[i].usage, commands[i].description);
}

static void version()
{
    fprintf(stderr, "swiss, version %s\n", SDK_VERSION);
}

int main(int argc, char **argv)
{
    if (argc < 2 || !strcmp(argv[1], "-h")) {
        usage();
        return 0;
    }

    if (!strcmp(argv[1], "-v")) {
        version();
        return 0;
    }

    const unsigned numCommands = sizeof(commands) / sizeof(commands[0]);
    for (unsigned i = 0; i < numCommands; ++i) {
        if (!strcmp(argv[1], commands[i].name))
            return commands[i].run(argc - 1, argv + 1);
    }

    fprintf(stderr, "no command named %s\n", argv[1]);
    usage();

    return 1;
}

