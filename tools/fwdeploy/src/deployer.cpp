#include "deployer.h"
#include "encrypter.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fstream>

using namespace std;

// in sync with firmware/master/stm32/board.h
const unsigned Deployer::VALID_HW_REVS[] = {
    2,  // BOARD_TC_MASTER_REV2
    3,  // BOARD_TEST_JIG
    4,  // BOARD_TC_MASTER_REV3
};

bool Deployer::hwRevIsValid(unsigned rev) {
    for (unsigned i = 0; i < arraysize(VALID_HW_REVS); ++i) {
        if (rev == VALID_HW_REVS[i]) {
            return true;
        }
    }

    return false;
}

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
bool Deployer::deploy(ContainerDetails &container)
{
    ofstream fout(container.outPath.c_str(), ofstream::binary);
    if (!fout.is_open()) {
        fprintf(stderr, "error: can't open %s (%s)\n", container.outPath.c_str(), strerror(errno));
        return false;
    }

    // magic number is first
    const uint64_t magic = MAGIC;
    if (fout.write((const char*)&magic, sizeof(magic)).fail()) {
        return false;
    }

    for (vector<FwDetails*>::iterator it = container.firmwares.begin();
         it != container.firmwares.end(); ++it)
    {
        FwDetails *fw = *it;

        if (!hwRevIsValid(fw->hwRev)) {
            fprintf(stderr, "unsupported hw rev specified: %d\n", fw->hwRev);
            return false;
        }

        Encrypter enc;
        if (!enc.encryptFile(fw->path.c_str(), fout)) {
            return false;
        }
    }

    fout.close();

    printStatus(container);

    return true;
}

void Deployer::printStatus(ContainerDetails &container)
{
    printf("Deploying %s (%s)\n", container.outPath.c_str(), container.fwVersion.c_str());
    for (vector<FwDetails*>::iterator it = container.firmwares.begin();
         it != container.firmwares.end(); ++it)
    {
        FwDetails *fw = *it;
        printf("  fw: %s, hw rev %d\n", fw->path.c_str(), fw->hwRev);
    }
}
