#ifndef ENCRYPTER_H
#define ENCRYPTER_H

#include <stdint.h>
#include <stdio.h>

#include <iostream>

class Encrypter
{
public:
    Encrypter();

    bool encryptFile(const char *inPath, std::ostream& os);

private:
    bool detailsForFile(FILE *f, uint32_t &sz, uint32_t &crc);
    bool encryptFWBinary(FILE *fin, std::ostream& os);
    void patchBlock(unsigned address, uint8_t *block, unsigned len);

    static const unsigned SALT_OFFSET = 7*4;
    static const unsigned SALT_LEN = 4*4;

    uint8_t salt[SALT_LEN];
};

#endif // ENCRYPTER_H
