#ifndef ENCRYPTER_H
#define ENCRYPTER_H

#include <stdint.h>
#include <stdio.h>

class Encrypter
{
public:
    Encrypter();

    bool encryptFile(const char *inPath, FILE *fout);

private:
    bool detailsForFile(FILE *f, uint32_t &sz, uint32_t &crc);
    bool encryptFWBinary(FILE *fin, FILE *fout);
    void patchBlock(unsigned address, uint8_t *block, unsigned len);

    static const unsigned SALT_OFFSET = 7*4;
    static const unsigned SALT_LEN = 4*4;

    uint8_t salt[SALT_LEN];
};

#endif // ENCRYPTER_H
