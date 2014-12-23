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

#ifndef BACKUP_H
#define BACKUP_H

#include "iodevice.h"
#include "usbvolumemanager.h"

#include <string>

class Backup
{
public:
    Backup(IODevice &_dev);

    static int run(int argc, char **argv, IODevice &_dev);

    int backup(const char *path);

private:
    IODevice &dev;

    unsigned requestProgress;
    unsigned replyProgress;

    static const unsigned DEVICE_SIZE = 16*1024*1024;
    static const unsigned PAGE_SIZE = 256;
    static const unsigned BLOCK_SIZE = 64*1024;

    bool writeFileHeader(FILE *f);
    bool writeFlashContents(FILE *f);

    bool sendRequest();
    bool writeReply(FILE *f);
};

#endif // BACKUP_H
