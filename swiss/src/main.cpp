
#include "command.h"
#include "profiler.h"
#include "fwloader.h"
#include "installer.h"
#include "manifest.h"
#include "delete.h"
#include "paircube.h"
#include "usbdevice.h"
#include "macros.h"

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
        "update",
        "update the firmware on your Sifteo Base",
        "update <firmware.sft>",
        FwLoader::run
    },
    {
        "install",
        "install a new game to the Sifteo Base",
        "install [-l] <app.elf>",
        Installer::run
    },
    {
        "manifest",
        "summarize the installed content on a Sifteo Base",
        "manifest",
        Manifest::run
    },
    {
        "delete",
        "delete data from the Sifteo Base",
        "delete (--all | --sys | volume)",
        Delete::run
    },
    {
        "pair",
        "add a pairing record to the Sifteo Base",
        "pair (--read | slotID hardwareID)",
        PairCube::run
    },
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
    fprintf(stderr, "swiss, " TOSTRING(SDK_VERSION) "\n");
}

static int run(int argc, char **argv, UsbDevice &usbdev)
{
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
    Usb::init();
    UsbDevice usbdev;
    int rv = run(argc, argv, usbdev);

    if (usbdev.isOpen()) {
        usbdev.close();
        while (usbdev.isOpen())
            usbdev.processEvents();
    }

    Usb::deinit();
    return rv;
}

