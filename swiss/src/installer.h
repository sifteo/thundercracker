/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
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

#ifndef INSTALLER_H
#define INSTALLER_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

class Installer
{
public:
    Installer(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    int install(const char *path, int vid, int pid, bool launcher, bool forceLauncher, bool rpc);

    static unsigned getInstallableElfSize(FILE *f);

private:
    int sendHeader(uint32_t filesz);
    bool getPackageMetadata(const char *path);
    bool sendFileContents(FILE *f, uint32_t filesz);
    bool commit();

    IODevice &dev;
    std::string package, version;
    bool isLauncher;
    bool isRPC;
};

#endif // INSTALLER_H
