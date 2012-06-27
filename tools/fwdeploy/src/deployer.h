#ifndef DEPLOYER_H
#define DEPLOYER_H

#include <stdio.h>
#include <stdint.h>

class Deployer
{
public:
    Deployer();

    bool deploy(const char *inPath, const char *outPath);

private:
    bool crcForFile(FILE *f, uint32_t &crc);
    bool encryptFWBinary(FILE *fin, FILE *fout);
};

#endif // DEPLOYER_H
