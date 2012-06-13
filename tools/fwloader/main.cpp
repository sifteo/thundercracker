
#include <stdio.h>
#include <string.h>

#include "usbdevice.h"
#include "loader.h"

static void usage()
{
    fprintf(stderr, "fwloader <firmware.bin>\n");
}

static void version()
{
    fprintf(stderr, "fwloader version\n");
}

int main(int argc, char **argv)
{
    char *path = 0;

    for (unsigned i = 0; i < argc; ++i) {

        if (!strcmp(argv[i], "-h")) {
            usage();
            return 0;
        }

        if (!strcmp(argv[i], "-v")) {
            version();
            return 0;
        }

        path = argv[i];
    }

    if (!path) {
        usage();
        return 1;
    }

    UsbDevice::init();

    const int vendorID = 0x22fa;
    const int productID = 0x0115;

    Loader loader;
    bool success = loader.load(path, vendorID, productID);
    return success ? 0 : 1;
}

