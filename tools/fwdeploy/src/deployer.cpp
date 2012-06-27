#include "deployer.h"
#include "crc.h"
#include "aes128.h"

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
 *
 * File format looks like:
 * - uint64_t magic number
 * - ... encrypted data ...
 * - uint32_t crc of the plaintext
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

    // prepend magic number
    const uint64_t magic = MAGIC;
    if (fwrite(&magic, 1, sizeof(uint64_t), fout) != sizeof(magic))
        return false;

    if (!encryptFWBinary(fin, fout))
        return false;

    // append the CRC
    if (fwrite(&calculatedCrc, 1, sizeof(uint32_t), fout) != sizeof(uint32_t))
        return false;

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

#define AES_IV  {  0x00, 0x01, 0x02, 0x03, \
                    0x04, 0x05, 0x06, 0x07, \
                    0x08, 0x09, 0x0a, 0x0b, \
                    0x0c, 0x0d, 0x0e, 0x0f }
#define AES_KEY { 0x2b7e1516, 0x28aed2a6, 0xabf71588, 0x09cf4f3c }

bool Deployer::encryptFWBinary(FILE *fin, FILE *fout)
{
    uint32_t expkey[44];
    const uint32_t key[4] = AES_KEY;
    AES128::expandKey(expkey, key);

    fseek(fin, 0L, SEEK_END);
    unsigned numPlainBytes = ftell(fin);
    rewind(fin);

    // cipherBuf holds the cipher in progress - starts with initialization vector
    uint8_t cipherBuf[AES128::BLOCK_SIZE] = AES_IV;
    uint8_t plainBuf[AES128::BLOCK_SIZE];

    while (numPlainBytes >= AES128::BLOCK_SIZE) {
        AES128::encryptBlock(cipherBuf, cipherBuf, expkey);

        unsigned numBytes = fread(plainBuf, 1, sizeof plainBuf, fin);
        if (numBytes != sizeof plainBuf) {
            fprintf(stderr, "error reading from file to encrypt: %s\n", strerror(errno));
            return false;
        }

        AES128::xorBlock(cipherBuf, plainBuf);

        numBytes = fwrite(cipherBuf, 1, AES128::BLOCK_SIZE, fout);
        if (numBytes != AES128::BLOCK_SIZE) {
            fprintf(stderr, "error writing to encrypted file: %s\n", strerror(errno));
            return false;
        }

        numPlainBytes -= AES128::BLOCK_SIZE;
    }

    /*
     * Pad with the number of leftover bytes, PKCS style.
     * Degenerate case is 16-byte aligned - have to do an entire block of nothing but pad
     */

    // assemble the final block - remaining plaintext plus padding
    ASSERT(numPlainBytes < AES128::BLOCK_SIZE);
    uint8_t padvalue = AES128::BLOCK_SIZE - numPlainBytes;
    if (fread(plainBuf, 1, numPlainBytes, fin) != numPlainBytes) {
        fprintf(stderr, "error reading from input file: %s\n", strerror(errno));
        return false;
    }
    memset(plainBuf + numPlainBytes, padvalue, padvalue);

    // last block
    AES128::encryptBlock(cipherBuf, cipherBuf, expkey);
    AES128::xorBlock(cipherBuf, plainBuf);
    if (fwrite(cipherBuf, 1, AES128::BLOCK_SIZE, fout) != AES128::BLOCK_SIZE) {
        fprintf(stderr, "error writing to output file: %s\n", strerror(errno));
        return false;
    }

    return true;
}
