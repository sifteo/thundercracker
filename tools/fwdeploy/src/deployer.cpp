#include "deployer.h"
#include "crc.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

Deployer::Deployer()
{
}

/*
 * To deploy a firmware image, we need to:
 * - calculate the CRC of the decrypted firmware image
 * - encrypt the firmware image
 * - ideally, ensure that the firmware image has been built in BOOTLOADABLE mode.
 */
bool Deployer::deploy(const char *inPath, const char *outPath)
{
    FILE *fin = fopen(inPath, "rb");
    if (!fin) {
        fprintf(stderr, "error: can't open %s (%s)\n", inPath, strerror(errno));
        return false;
    }

    FILE *fout = fopen(outPath, "wb");
    if (!fout) {
        fprintf(stderr, "error: can't open %s (%s)\n", outPath, strerror(errno));
        return false;
    }

    Crc32::init();

    uint32_t calculatedCrc;
    if (!crcForFile(fin, calculatedCrc))
        return false;

    rewind(fin);
    if (!encryptFWBinary(fin, fout, calculatedCrc))
        return false;

    fprintf(stderr, "calculated CRC: %lu\n", calculatedCrc);

    fclose(fin);
    fclose(fout);

    return true;
}

/*
 * Calculate the CRC for the input file.
 * This is read-only
 */
bool Deployer::crcForFile(FILE *f, uint32_t &crc)
{
    Crc32::reset();

    while (!feof(f)) {
        uint8_t crcbuffer[32];
        const int numBytes = fread(crcbuffer, 1, sizeof crcbuffer, f);
        if (numBytes <= 0)
            continue;

        // must be word aligned
        ASSERT((numBytes & 0x3) == 0 && "firmware binary must be word aligned");

        const uint32_t *p = reinterpret_cast<const uint32_t*>(crcbuffer);
        const uint32_t *end = reinterpret_cast<const uint32_t*>(crcbuffer + numBytes);
        while (p < end)
            Crc32::add(*p++);
    }

    crc = Crc32::get();
    return true;
}

/*
 * Right now, we're just writing the input file straight through unencrypted.
 */
bool Deployer::encryptFWBinary(FILE *fin, FILE *fout, uint32_t crc)
{
    while (!feof(fin)) {
        uint8_t buffer[512];
        int numBytes = fread(buffer, 1, sizeof(buffer), fin);
        if (numBytes <= 0)
            break;

        if (fwrite(buffer, 1, numBytes, fout) != numBytes)
            return false;
    }

    // append the CRC to the firmware image - normally this will be part of the
    // encrypted blob
    if (fwrite(&crc, 1, sizeof(uint32_t), fout) != 4)
        return false;

    // temp check that file sizes make sense
    fseek(fin, 0L, SEEK_END);
    fseek(fout, 0L, SEEK_END);
    unsigned foutsize = ftell(fout);
    ASSERT(ftell(fin) == (foutsize - sizeof(uint32_t)) && "file sizes don't match");

    return true;
}
