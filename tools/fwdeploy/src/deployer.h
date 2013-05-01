#ifndef DEPLOYER_H
#define DEPLOYER_H

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <vector>

class Deployer
{
public:

    static const uint64_t MAGIC = 0x5857465874666953ULL;    // original version
    static const uint64_t MAGIC2 = 0x5857465874666953ULL;   // archive version

    Deployer();

    // metadata for the entire archive
    enum ContainerMetadata {
        FirmwareRev
    };

    // metadata for each enclosed firmware
    enum ElementMetadata {
        HardwareRev,
        FirmwareBlob
    };

    struct MetadataHeader {
        unsigned key;
        unsigned size;
    };

    // details for each embedded firmware version
    struct FwDetails {
        unsigned hwRev;
        std::string path;

        FwDetails(unsigned r, const std::string &p) :
            hwRev(r), path(p)
        {}
    };

    struct ContainerDetails {
        std::string outPath;
        std::string fwVersion;
        std::vector<Deployer::FwDetails *> firmwares;

        bool isValid() const {

            if (fwVersion.empty()) {
                fprintf(stderr, "error: firmware version not specified\n");
                return false;
            }

            if (firmwares.empty()) {
                fprintf(stderr, "error: no firmwares specified\n");
                return false;
            }

            return true;
        }
    };

    bool deploy(const char *inPath, const char *outPath);

private:

    static const unsigned VALID_HW_REVS[];

    bool detailsForFile(FILE *f, uint32_t &sz, uint32_t &crc);
    bool encryptFWBinary(FILE *fin, FILE *fout);
    void patchBlock(unsigned address, uint8_t *block, unsigned len);

    static const unsigned SALT_OFFSET = 7*4;
    static const unsigned SALT_LEN = 4*4;

    uint8_t salt[SALT_LEN];
};

#endif // DEPLOYER_H
