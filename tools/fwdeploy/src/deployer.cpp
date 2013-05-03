#include "deployer.h"
#include "encrypter.h"
#include "macros.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fstream>
#include <sstream>

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


bool Deployer::deploy(Container &container)
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

    if (!writeSection(FirmwareRev, container.fwVersion.length(), container.fwVersion.c_str(), fout)) {
        return false;
    }

    stringstream ss;
    if (!encryptFirmwares(container, ss)) {
        return false;
    }

    long pos = ss.tellp();
    if (!writeSection(FirmwareBinaries, pos, ss.str().c_str(), fout)) {
        return false;
    }

    fout.close();

    printStatus(container);

    return true;
}

bool Deployer::encryptFirmwares(Container &container, ostream &os)
{
    /*
     * For each firmware enclosed in the container,
     * write both the hardware rev and the firmware blob itself.
     */

    for (vector<Firmware*>::iterator it = container.firmwares.begin();
         it != container.firmwares.end(); ++it)
    {
        Firmware *fw = *it;

        // write hardware rev
        if (!hwRevIsValid(fw->hwRev)) {
            fprintf(stderr, "unsupported hw rev specified: %d\n", fw->hwRev);
            return false;
        }

        if (!writeSection(HardwareRev, sizeof fw->hwRev, &fw->hwRev, os)) {
            return false;
        }

        // write firmware blob itself
        Encrypter enc;
        stringstream ss;
        if (!enc.encryptFile(fw->path.c_str(), ss)) {
            return false;
        }

        long pos = ss.tellp();
        if (!writeSection(FirmwareBlob, pos, ss.str().c_str(), os)) {
            return false;
        }
    }

    return true;
}

bool Deployer::writeSection(uint32_t key, uint32_t size, const void *bytes, ostream& os)
{
    // XXX: endianness

    if (os.write((const char*)&key, sizeof key).fail()) {
        return false;
    }

    if (os.write((const char*)&size, sizeof size).fail()) {
        return false;
    }

    if (os.write((const char*)bytes, size).fail()) {
        return false;
    }

    return true;
}

void Deployer::printStatus(Container &container)
{
    printf("Deploying %s (%s)\n", container.outPath.c_str(), container.fwVersion.c_str());
    for (vector<Firmware*>::iterator it = container.firmwares.begin();
         it != container.firmwares.end(); ++it)
    {
        Firmware *fw = *it;
        printf("  fw: %s, hw rev %d\n", fw->path.c_str(), fw->hwRev);
    }
}
