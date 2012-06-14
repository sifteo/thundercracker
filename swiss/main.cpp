
#include "command.h"
#include "profiler.h"
#include "fwloader.h"
#include "usbdevice.h"

#include <stdio.h>
#include <string.h>

static const Command commands[] = {
    {
        "profile",
        "capture profiling data from an app",
        "profile <app.elf> <output.txt>",
        Profiler::run
    },
    {
        "fwload",
        "install new firmware to the Sifteo Base",
        "fwload <firmware.bin>",
        FwLoader::run
    }
};

static void usage()
{
    const unsigned numCommands = sizeof(commands) / sizeof(commands[0]);

    unsigned maxUsageLen = 0;
    for (unsigned i = 0; i < numCommands; ++i) {
        if (strlen(commands[i].usage) > maxUsageLen)
            maxUsageLen = strlen(commands[i].usage);
    }
    maxUsageLen += 4;

    fprintf(stderr, "swiss - your Sifteo utility knife\n");
    for (unsigned i = 0; i < numCommands; ++i) {
        unsigned pad = maxUsageLen - strlen(commands[i].usage);
        fprintf(stderr, "  %s:%*s%s\n", commands[i].usage, pad, " ", commands[i].description);
    }
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

    /*
     * TODO: add support for specifying a TCP connection to siftulator.
     */
    UsbDevice::init();
    UsbDevice usbdev;

    const unsigned numCommands = sizeof(commands) / sizeof(commands[0]);
    const char *commandName = argv[1];

    for (unsigned i = 0; i < numCommands; ++i) {
        if (!strcmp(commandName, commands[i].name))
            return commands[i].run(argc - 1, argv + 1, usbdev);
    }

    fprintf(stderr, "no command named %s\n", commandName);
    usage();

    return 1;
}

