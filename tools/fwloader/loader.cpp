#include "loader.h"
#include "usbdevice.h"

#include <stdio.h>
#include <errno.h>
#include <string.h>

Loader::Loader()
{
}

bool Loader::load(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "could not open %s: %s\n", path, strerror(errno));
        return false;
    }

    const int vendorID = 0x22fa;
    const int productID = 0x0110;

    UsbDevice dev;
    if (!dev.open(vendorID, productID)) {
        fprintf(stderr, "device is not attached\n");
        return false;
    }

    return true;
}
