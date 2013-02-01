
#include "command.h"
#include "profiler.h"
#include "fwloader.h"
#include "installer.h"
#include "manifest.h"
#include "delete.h"
#include "paircube.h"
#include "usbdevice.h"
#include "reboot.h"
#include "macros.h"
#include "backup.h"
#include "savedata.h"
#include "listen.h"

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
        "install",
        "install a new game to the Sifteo Base",
        "install [-l] <app.elf>",
        Installer::run
    },
    {
        "listen",
        "listen for and decode log activity from the Sifteo base",
        "listen <app.elf> [--fout <file.txt>]",
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
        "update (--init | <firmware.sft>)",
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

static int run(int argc, char **argv, UsbDevice &usbdev)
{
    const unsigned numCommands = sizeof(commands) / sizeof(commands[0]);

    // command is first arg, fixed position
    const char *commandName = argv[0];

    for (unsigned i = 0; i < numCommands; ++i) {
        if (!strcmp(commandName, commands[i].name))
            return commands[i].run(argc, argv, usbdev);
    }

    fprintf(stderr, "no command named %s\n", commandName);
    usage();

    return 1;
}

static unsigned handleGlobalArgs(int argc, char **argv)
{
    /*
     * Handle global args, not specific to any command.
     *
     * Return the number of args consumed - this will always include argv[0],
     * such that we're pointing to the first command by the time we're done.
     */

    unsigned consumed = 1; // consume argv[0]
    for (unsigned i = 1; i < argc; ++i) {

        if (!strcmp(argv[i], "--libusb-debug") && i + 1 < argc) {

            unsigned long level = strtoul(argv[i + 1], NULL, 0);
            Usb::setDebug(level);

            consumed += 2;
            i++;
            continue;
        }

    }

    return consumed;
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

    unsigned consumed = handleGlobalArgs(argc, argv);
    argc -= consumed;
    argv += consumed;

    if (argc == 0) {
        usage();
        return 1;
    }

    UsbDevice usbdev;
    int rv = run(argc, argv, usbdev);

    if (usbdev.isOpen()) {
        usbdev.close();
        while (usbdev.isOpen())
            usbdev.processEvents(1);
    }

    Usb::deinit();
    return rv;
}

