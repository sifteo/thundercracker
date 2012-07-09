#ifndef DEPLOYER_H
#define DEPLOYER_H

#include <stdio.h>
#include <stdint.h>

class Deployer
{
public:

    static const uint64_t MAGIC = 0x5857465874666953ULL;

    Deployer();

    bool deploy(const char *inPath, const char *outPath);

private:
    bool detailsForFile(FILE *f, uint32_t &sz, uint32_t &crc);
    bool encryptFWBinary(FILE *fin, FILE *fout);
    void patchBlock(unsigned address, uint8_t *block, unsigned len);

    static const unsigned SALT_OFFSET = 7*4;
    static const unsigned SALT_LEN = 4*4;

    uint8_t salt[SALT_LEN];
};

#endif // DEPLOYER_H
