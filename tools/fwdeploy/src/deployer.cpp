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
    /*
     * File format:
     *  uint64_t magic number
     *  uint32_t file format version
     *
     *  (
     *    uint32_t header key
     *    uint32_t header size
     *    uint8_t[header size] value
     *  )
     */

    ofstream fout(container.outPath.c_str(), ofstream::binary);
    if (!fout.is_open()) {
        fprintf(stderr, "error: can't open %s (%s)\n", container.outPath.c_str(), strerror(errno));
        return false;
    }

    // magic number is first
    const uint64_t magic = MAGIC_CONTAINER;
    if (fout.write((const char*)&magic, sizeof magic).fail()) {
        return false;
    }

    // file format version
    const uint32_t fileVersion = FILE_VERSION;
    if (fout.write((const char*)&fileVersion, sizeof fileVersion).fail()) {
        return false;
    }

    if (!writeSection(FirmwareRev, container.fwVersion.length(), container.fwVersion.c_str(), fout)) {
        return false;
    }

    for (vector<Firmware*>::iterator it = container.firmwares.begin();
         it != container.firmwares.end(); ++it)
    {
        stringstream ss;
        Firmware *fw = *it;

        if (!encryptFirmware(fw, ss)) {
            return false;
        }

        long pos = ss.tellp();
        if (!writeSection(FirmwareBinary, pos, ss.str().c_str(), fout)) {
            return false;
        }
    }

    fout.close();

    printStatus(container);

    return true;
}

bool Deployer::deploySingle(const char *inPath, const char *outPath)
{
    /*
     * Earlier versions of fwdeploy packaged only a single firmware.
     * Retain compatibility in case we need to generate those at some point.
     */

    ofstream fout(outPath, ofstream::binary);
    if (!fout.is_open()) {
        fprintf(stderr, "error: can't open %s (%s)\n", outPath, strerror(errno));
        return false;
    }

    const uint64_t magic = MAGIC;
    if (fout.write((const char*)&magic, sizeof magic).fail()) {
        return false;
    }

    Encrypter enc;
    if (!enc.encryptFile(inPath, fout)) {
        return false;
    }

    fout.close();

    return true;
}

bool Deployer::encryptFirmware(Firmware *firmware, ostream &os)
{
    /*
     * Encrypt a single Firmware image to the stream, preceded
     * by its hardware rev.
     */

    // write hardware rev
    if (!hwRevIsValid(firmware->hwRev)) {
        fprintf(stderr, "unsupported hw rev specified: %d\n", firmware->hwRev);
        return false;
    }

    if (!writeSection(HardwareRev, sizeof firmware->hwRev, &firmware->hwRev, os)) {
        return false;
    }

    // write firmware blob itself
    Encrypter enc;
    stringstream ss;
    if (!enc.encryptFile(firmware->path.c_str(), ss)) {
        return false;
    }

    long pos = ss.tellp();
    if (!writeSection(FirmwareBlob, pos, ss.str().c_str(), os)) {
        return false;
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
