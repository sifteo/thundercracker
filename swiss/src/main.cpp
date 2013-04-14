
#include "command.h"
#include "profiler.h"
#include "fwloader.h"
#include "inspect.h"
#include "installer.h"
#include "manifest.h"
#include "delete.h"
#include "paircube.h"
#include "usbdevice.h"
#include "netdevice.h"
#include "reboot.h"
#include "macros.h"
#include "backup.h"
#include "savedata.h"
#include "listen.h"
#include "options.h"

#include <stdio.h>
#include <string.h>

static const Command commands[] = {
    // Keeping this list in alphabetical order, for lack of a better ordering...

    {
        "backup",
        "create a full backup of a Sifteo Base filesystem",
        "backup <filesystem.bin>",
        Backup::run
    },
    {
        "delete",
        "delete data from the Sifteo Base",
        "delete (--all | --sys | --reformat | <volumeID>)",
        Delete::run
    },
    {
        "inspect",
        "inspect an app's metadata",
        "inspect <app.elf>",
        Inspect::run
    },
    {
        "install",
        "install a new game to the Sifteo Base",
        "install [-l] <app.elf>",
        Installer::run
    },
    {
        "listen",
        "listen for and decode log activity from the Sifteo base",
        "listen <app.elf> [--fout <file.txt> | --flush-logs]",
        Listen::run
    },
    {
        "manifest",
        "summarize the installed content on a Sifteo Base",
        "manifest",
        Manifest::run
    },
    {
        "pair",
        "add a pairing record to the Sifteo Base",
        "pair (--read | slotID hardwareID)",
        PairCube::run
    },
    {
        "profile",
        "capture profiling data from an app",
        "profile <app.elf> <output.txt>",
        Profiler::run
    },
    {
        "reboot",
        "reset the Sifteo Base, equivalent to reinserting batteries",
        "reboot",
        Reboot::run
    },
    {
        "savedata",
        "extract or restore an application's save data",
        "savedata (extract <package> <fout> [--raw] | restore <fin>)",
        SaveData::run
    },
    {
        "update",
        "update the firmware on your Sifteo Base",
        "update (--init | --pid <pid> | <firmware.sft>)",
        FwLoader::run
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
#if defined(SWISS_LINUX)
    fprintf(stderr, "swiss " TOSTRING(SDK_VERSION) "\n");
#else
    const struct libusb_version *v = libusb_get_version();
    fprintf(stderr, "swiss " TOSTRING(SDK_VERSION) ", libusb v%d.%d.%d.%d\n", v->major, v->minor, v->micro, v->nano);
#endif
}

static int run(int argc, char **argv, IODevice &iodev)
{
    const unsigned numCommands = sizeof(commands) / sizeof(commands[0]);

    // command is first arg, fixed position
    const char *commandName = argv[0];

    for (unsigned i = 0; i < numCommands; ++i) {
        if (!strcmp(commandName, commands[i].name))
            return commands[i].run(argc, argv, iodev);
    }

    fprintf(stderr, "no command named %s\n", commandName);
    usage();

    return 1;
}

static int runNet(int argc, char **argv)
{
    NetDevice netdev;
    int rv = run(argc, argv, netdev);
    netdev.close();

    return rv;
}

static int runUsb(int argc, char **argv)
{
    Usb::init();
    UsbDevice usbdev;

    int rv = run(argc, argv, usbdev);

    if (usbdev.isOpen()) {
        usbdev.close();
        while (usbdev.isOpen()) {
            usbdev.processEvents(1);
        }
    }

    Usb::deinit();

    return rv;
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

    unsigned consumed = Options::processGlobalArgs(argc, argv);
    argc -= consumed;
    argv += consumed;

    if (argc == 0) {
        usage();
        return 1;
    }

    if (Options::useNetDevice()) {
        return runNet(argc, argv);
    }

    return runUsb(argc, argv);
}

