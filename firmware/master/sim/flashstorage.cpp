#include "flash.h"
#include "flashstorage.h"
#include <sifteo.h>

#include <stdio.h>
#include <string.h>

const char *FlashStorage::filename = "flashstorage.bin";

void FlashStorage::init()
{
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        chipErase();
    }
}

void FlashStorage::read(uint32_t address, uint8_t *buf, unsigned len)
{
    FILE *file = fopen(filename, "rb");
    ASSERT(file != NULL && "ERROR: FlashLayer failed to open asset segment - make sure asegment.bin is next to your executable.\n");
    if (fseek(file, address, SEEK_SET) != 0) {
        LOG(("FlashStorage::read() seek error\n"));
    }
    unsigned rxed = fread(buf, 1, len, file);
    if (rxed != len) {
        LOG(("FlashStorage: read error, got %d expected %d\n", rxed, len));
    }
}

bool FlashStorage::write(uint32_t address, const uint8_t *buf, unsigned len)
{
    // TODO: verify file exists
    FILE *file = fopen(filename, "wb");
    ASSERT(fseek(file, address, SEEK_SET) == 0 && "FlashStorage::write() - seek err\n");

    unsigned written = fwrite(buf, 1, len, file);
    if (written != len) {
        LOG(("FlashStorage::write() - write err, got %d expected %d\n", written, len));
        return false;
    }
    return true;
}

bool FlashStorage::eraseSector(uint32_t address)
{
    return false;
}

bool FlashStorage::chipErase()
{
    FILE *file = fopen(filename, "wb");
    ASSERT(file != NULL);

    uint8_t filler[Flash::SECTOR_SIZE];
    memset(filler, 0xFF, sizeof(filler));

    for (unsigned i = 0; i < Flash::CAPACITY; i += Flash::SECTOR_SIZE) {
        unsigned written = fwrite(filler, 1, sizeof(filler), file);
        if (written != sizeof(filler)) {
            LOG(("FlashStorage: write error, wrote %d expected %d\n", written, Flash::SECTOR_SIZE));
            return false;
        }
    }
    fclose(file);
    return true;
}

