/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware deployment tool
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef DEPLOYER_H
#define DEPLOYER_H

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <vector>

class Deployer
{
public:

    static const uint64_t MAGIC = 0x5857465874666953ULL;            // original version
    static const uint64_t MAGIC_CONTAINER = 0x5A57465874666953ULL;  // archive version

    static const uint32_t FILE_VERSION = 1;

    Deployer();

    // metadata for the entire archive
    enum ContainerMetadata {
        FirmwareRev,
        FirmwareBinary
    };

    // metadata for each enclosed firmware
    enum ElementMetadata {
        HardwareRev,
        FirmwareBlob
    };

    struct MetadataHeader {
        uint32_t key;
        uint32_t size;
    };

    // details for each embedded firmware version
    struct Firmware {
        uint32_t hwRev;
        std::string path;

        Firmware(unsigned r, const std::string &p) :
            hwRev(r), path(p)
        {}
    };

    struct Container {
        std::string outPath;
        std::string fwVersion;
        std::vector<Deployer::Firmware*> firmwares;

        bool isValid() const {

            if (fwVersion.empty()) {
                return false;
            }

            if (firmwares.empty()) {
                return false;
            }

            return true;
        }
    };

    bool deploy(Container &container);
    bool deploySingle(const char *inPath, const char *outPath);

    static bool writeSection(uint32_t key, uint32_t size, const void *bytes, std::ostream &os);

private:
    static const unsigned VALID_HW_REVS[];
    static bool hwRevIsValid(unsigned rev);
    static void printStatus(Container &container);

    bool encryptFirmware(Firmware *firmware, std::ostream &os);
};

#endif // DEPLOYER_H
