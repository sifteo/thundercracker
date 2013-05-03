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
        FirmwareRev,
        FirmwareBinaries
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
        uint32_t hwRev;
        std::string path;

        FwDetails(unsigned r, const std::string &p) :
            hwRev(r), path(p)
        {}
    };

    struct ContainerDetails {
        std::string outPath;
        std::string fwVersion;
        std::vector<Deployer::FwDetails*> firmwares;

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

    bool deploy(ContainerDetails &container);

    static bool writeSection(uint32_t key, uint32_t size, const void *bytes, std::ostream &os);

private:
    static const unsigned VALID_HW_REVS[];
    static bool hwRevIsValid(unsigned rev);
    static void printStatus(ContainerDetails &container);

    bool encryptFirmwares(ContainerDetails &container, std::ostream &os);
};

#endif // DEPLOYER_H
