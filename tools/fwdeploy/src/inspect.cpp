#include "inspect.h"
#include "deployer.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>


#include <fstream>

using namespace std;

Inspect::Inspect()
{
}

void Inspect::dumpSections(const char *path)
{
    ifstream fin(path, ofstream::binary);
    if (!fin.is_open()) {
        fprintf(stderr, "couldn't open %s (%s)\n", path, strerror(errno));
        return;
    }

    uint64_t magic;
    if (fin.read((char*)&magic, sizeof magic).fail()) {
        return;
    }

    if (magic == Deployer::MAGIC) {
        printf("single encrpyted binary\n");
        return;
    }

    if (magic != Deployer::MAGIC_CONTAINER) {
        fprintf(stderr, "unrecognized magic number\n");
        return;
    }

    uint32_t fileFormatVersion;
    if (fin.read((char*)&fileFormatVersion, sizeof fileFormatVersion).fail()) {
        return;
    }
    cout << "File Format Version: " << fileFormatVersion << endl;

    dumpContainer(fin);
}

void Inspect::dumpContainer(std::istream &is)
{
    for (;;) {

        Deployer::MetadataHeader hdr;
        if (is.read((char*)&hdr, sizeof hdr).fail()) {
            if (!is.eof()) {
                cerr << "fin read err" << endl;
            }
            return;
        }

        switch (hdr.key) {

        case Deployer::FirmwareRev: {
            string version(hdr.size, '\0');
            if (is.read(&version[0], hdr.size).fail()) {
                return;
            }
            cout << "Firmware Version: " << version << endl;
            break;
        }

        case Deployer::FirmwareBinary:
            if (!dumpFirmware(is, hdr.size)) {
                return;
            }
            break;

        default:
            cerr << "unrecognized Container header: " << hdr.key << endl;
            is.seekg(hdr.size);
            break;
        }
    }
}

bool Inspect::dumpFirmware(std::istream &is, unsigned sectionLen)
{
    cout << "Firmware Image" << endl;

    int64_t endOfSection = static_cast<int64_t>(is.tellg()) + sectionLen;

    while (is.tellg() < endOfSection) {

        Deployer::MetadataHeader hdr;
        if (is.read((char*)&hdr, sizeof hdr).fail()) {
            return false;
        }

        switch (hdr.key) {

        case Deployer::HardwareRev: {
            uint32_t hwRev;

            if (hdr.size != sizeof hwRev) {
                cerr << "invalid size for HardwareRev: " << hdr.size;
            }

            if (is.read((char*)&hwRev, sizeof hwRev).fail()) {
                return false;
            }

            cout << "  Hardware Rev: " << hwRev << endl;
            break;
        }

        case Deployer::FirmwareBlob:
            cout << "  Binary Blob: " << hdr.size << " bytes" << endl;
            is.seekg(hdr.size, is.cur);
            break;

        default:
            cerr << "unrecognized FirmwareBinaries header: " << hdr.key << endl;
            is.seekg(hdr.size, is.cur);
            break;
        }
    }

    if (is.tellg() != endOfSection) {
        cerr << "error: read to " << is.tellg() << ", wanted " << endOfSection << endl;
    }

    return true;
}
