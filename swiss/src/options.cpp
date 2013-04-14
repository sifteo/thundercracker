#include "options.h"
#include "usbdevice.h"

#include <stdio.h>
#include <string.h>

bool Options::useNetDev = false;

unsigned Options::processGlobalArgs(int argc, char **argv)
{
    /*
     * Handle global args, not specific to any command.
     *
     * Return the number of args consumed - this will always include argv[0],
     * such that we're pointing to the first command by the time we're done.
     */

    unsigned consumed = 1; // consume argv[0]
    for (int i = 1; i < argc; ++i) {

        if (!strcmp(argv[i], "--libusb-debug") && i + 1 < argc) {

            unsigned long level = strtoul(argv[i + 1], NULL, 0);
            Usb::setDebug(level);

            consumed += 2;
            i++;
            continue;
        }

        if (!strcmp(argv[i], "--net")) {
            useNetDev = true;
            consumed++;
            i++;
            continue;
        }

    }

    return consumed;
}
