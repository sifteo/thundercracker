#include "deployer.h"
#include "crc.h"
#include "aes128.h"
#include "securerandom.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

// in sync with firmware/master/stm32/board.h
const unsigned Deployer::VALID_HW_REVS[] = {
    2,  // BOARD_TC_MASTER_REV2
    3,  // BOARD_TEST_JIG
    4,  // BOARD_TC_MASTER_REV3
};

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
 * - uint32_t size of the plaintext
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

    if (!SecureRandom::generate(salt, SALT_LEN)) {
        fprintf(stderr, "error generating random salt: %s\n", strerror(errno));
        return false;
    }

    uint32_t plainsz, calculatedCrc;
    if (!detailsForFile(fin, plainsz, calculatedCrc))
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

    // append the plaintext size
    if (fwrite(&plainsz, 1, sizeof(uint32_t), fout) != sizeof(uint32_t))
        return false;

    fclose(fin);
    fclose(fout);

    return true;
}

/*
 * Calculate the CRC for the input file.
 * This is read-only
 */
bool Deployer::detailsForFile(FILE *f, uint32_t &sz, uint32_t &crc)
{
    const char elf[] = { 0x7f, 'E', 'L', 'F' };
    char header[sizeof elf];
    if (fread(header, 1, sizeof elf, f) != sizeof(elf))
        return false;
    if (!memcmp(elf, header, sizeof elf)) {
        fprintf(stderr, "error: looks like you're deploying a .elf, please convert it to .bin first\n");
        return false;
    }

    rewind(f);
    Crc32::reset();
    unsigned fileOffset = 0;

    while (!feof(f)) {
        uint8_t crcbuffer[32];
        const int numBytes = fread(crcbuffer, 1, sizeof crcbuffer, f);
        if (numBytes <= 0)
            continue;

        patchBlock(fileOffset, crcbuffer, numBytes);
        fileOffset += numBytes;

        // must be word aligned
        ASSERT((numBytes & 0x3) == 0 && "firmware binary must be word aligned");

        const uint32_t *p = reinterpret_cast<const uint32_t*>(crcbuffer);
        const uint32_t *end = reinterpret_cast<const uint32_t*>(crcbuffer + numBytes);
        while (p < end)
            Crc32::add(*p++);
    }

    sz = ftell(f);
    crc = Crc32::get();

    if (crc == 0 || crc == 0xffffffff) {
        fprintf(stderr, "error: this file has a CRC of 0 or 0xffffffff, which the bootloader considers invalid\n");
        return false;
    }

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
    unsigned fileOffset = 0;

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

        // Patch plaintext before encrypting it
        patchBlock(fileOffset, plainBuf, sizeof plainBuf);
        fileOffset += AES128::BLOCK_SIZE;

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

void Deployer::patchBlock(unsigned address, uint8_t *block, unsigned len)
{
    /*
     * This function is called once per block of plaintext, immediately prior to
     * encryption. Right now it just solves one problem I'm somewhat paranoid
     * about: If we release two master firmware binaries with a predictable
     * difference in their plaintext, that may make it significantly easier to
     * recover key material via differential cryptanalysis.
     *
     * To subvert this, we place some unpredictable (cryptographically secure
     * random) "salt" data near the beginning of the binary image. To do this
     * unobtrusively and without forcing any contortions in the bootloader,
     * we just splat it into a reserved portion of the IVT.
     */

    for (unsigned i = 0; i < len; ++i) {
        unsigned offset = address + i - SALT_OFFSET;
        if (offset < SALT_LEN)
            block[i] = salt[offset];
    }
}

