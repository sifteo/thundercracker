/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
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

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#else
#   include <sys/mman.h>
#   include <sys/stat.h>
#   include <fcntl.h>
#   include <errno.h>
#   include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "macros.h"
#include "flash_device.h"
#include "flash_storage.h"
#include "flash_volume.h"
#include "flash_recycler.h"
#include "ostime.h"
#include "lodepng.h"


// Compiled launcher ELF binary, installed in new volumes.
extern const uint8_t launcher[];


FlashStorage::FlashStorage()
    : data(NULL), isInitialized(false) {}
    
FlashStorage::~FlashStorage()
{
    if (isInitialized) {
        // Normally exit() is called, but this may happen in case of
        // unclean termination. We still need to flush and unmap the file.
        exit();
    }
}

bool FlashStorage::init(const char *filename)
{
    ASSERT(isInitialized == false);
    isFileBacked = filename != NULL;

    if (isFileBacked) {
        // Disk-backed flash memory
        if (!mapFile(filename))
            return false;
        if (!checkData()) {
            unmapFile();
            return false;
        }
    } else {
        // Anonymous non-persistent flash memory
        data = new FileRecord();
        initData();
    }

    isInitialized = true;
    return true;
}

void FlashStorage::exit()
{
    ASSERT(isInitialized == true);

    if (isFileBacked)
        unmapFile();
    else
        delete data;

    data = NULL;
    isInitialized = false;
}

void FlashStorage::initData()
{
    ASSERT(data);

    // Zero unused parts of the header
    memset(data->header.bytes, 0x00, sizeof data->header.bytes);

    initCubes();
    initMC();
    initHeader();
}

void FlashStorage::initCubes()
{
    for (unsigned i = 0; i < arraysize(data->cubes); ++i) {
        CubeRecord &cube = data->cubes[i];

        // Initialize internal (OTP) storage to its erased state
        memset(cube.nvm, 0xFF, sizeof cube.nvm);
    
        // Initialize external storage to all zeroes, so we force
        // the firmware to erase all areas before they're used. It shouldn't
        // be making assumptions about blocks being already-erased unless it
        // knows this for certain.
        memset(cube.ext, 0x00, sizeof cube.ext);

        // Zero out the erase counts
        memset(cube.eraseCounts, 0x00, sizeof cube.eraseCounts);
    }

    data->header.cube_count = arraysize(data->cubes);
    data->header.cube_nvmSize = sizeof data->cubes[0].nvm;
    data->header.cube_extSize = sizeof data->cubes[0].ext;
    data->header.cube_sectorSize = Cube::FlashModel::SECTOR_SIZE;
}

void FlashStorage::initMC()
{
    // Initialize internal flash to all zeroes, also to make sure nobody
    // relies on it starting out erased.
    memset(data->master.bytes, 0x00, sizeof data->master.bytes);

    // Zero out the erase counts
    memset(data->master.eraseCounts, 0x00, sizeof data->master.eraseCounts);

    data->header.mc_pageSize = FlashDevice::PAGE_SIZE;
    data->header.mc_capacity = FlashDevice::CAPACITY;
    data->header.mc_blockSize = FlashDevice::ERASE_BLOCK_SIZE;
}

void FlashStorage::initHeader()
{
    // Fill in the header...
    data->header.magic = HeaderRecord::MAGIC;
    data->header.version = HeaderRecord::CURRENT_VERSION;
    data->header.fileSize = sizeof *data;

    // Create a unique ID for this storage file
    data->header.uniqueID = rand() ^ rand() ^ uint32_t(OSTime::clock() * 1e6);
}

bool FlashStorage::installLauncher(const char *filename)
{
    /*
     * Erase any already-installed LAUNCHER binaries, and install a new
     * launcher using the specified file, or the built-in binary.
     */

    // Built-in launcher
    uint32_t launcherSize = *reinterpret_cast<const uint32_t*>(launcher);
    const uint8_t *launcherData = launcher + 4;
    
    std::vector<uint8_t> data;
    if (filename) {
        LodePNG::loadFile(data, filename);
        if (data.empty()) {
            LOG(("FLASH: Launcher binary '%s' cannot be read\n", filename));
            return false;
        }
        launcherSize = data.size();
        launcherData = &data[0];
    }

    FlashVolumeWriter writer;
    if (!writer.beginLauncher(launcherSize)) {
        LOG(("FLASH: Insufficient space for launcher binary\n"));
        return false;
    }

    writer.appendPayload(launcherData, launcherSize);
    writer.commit();
    return true;
}

bool FlashStorage::checkData()
{
    /*
     * In the future, we may have softer versioning for these storage files.
     * For now, require that everything matches our current version exactly.
     *
     * The only currently permissible variation: Cube data is optional.
     * If the file has cube_count==0, we ignore all cube data and reinitialize
     * it from scratch.
     */

    if (data->header.magic != HeaderRecord::MAGIC) {
        LOG(("FLASH: Storage file contains data in an unrecognized format\n"));
        return false;
    }

    if (data->header.version != HeaderRecord::CURRENT_VERSION) {
        LOG(("FLASH: Storage file has an unsupported version\n"));
        return false;
    }

    if (data->header.cube_count == 0) {
        // Importing a file with no cubes. Update cubes and header, leave MC alone.
        initHeader();
        initCubes();
        LOG(("FLASH: Importing storage file with no saved cube data\n"));
    }

    if (data->header.fileSize != sizeof *data ||
        data->header.cube_count != arraysize(data->cubes) ||
        data->header.cube_nvmSize != sizeof data->cubes[0].nvm ||
        data->header.cube_extSize != sizeof data->cubes[0].ext ||
        data->header.cube_sectorSize != Cube::FlashModel::SECTOR_SIZE ||
        data->header.mc_pageSize != FlashDevice::PAGE_SIZE ||
        data->header.mc_capacity != FlashDevice::CAPACITY ||
        data->header.mc_blockSize != FlashDevice::ERASE_BLOCK_SIZE) {
        LOG(("FLASH: Storage file has an unsupported memory layout\n"));
        return false;
    }

    return true;
}

bool FlashStorage::mapFile(const char *filename)
{
#ifdef _WIN32

    HANDLE fh = CreateFile(filename, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        LOG(("FLASH: Can't open backing file '%s' (%08x)\n",
            filename, (unsigned)GetLastError()));
        return false;
    }

    fileHandle = (uintptr_t) fh;
    bool newFile = GetFileSize(fh, NULL) == (DWORD)0;

    HANDLE mh = CreateFileMapping(fh, NULL, PAGE_READWRITE, 0, sizeof *data, NULL);
    if (mh == NULL) {
        CloseHandle(fh);
        LOG(("FLASH: Can't create mapping for file '%s' (%08x)\n",
            filename, (unsigned)GetLastError()));
        return false;
    }
    mappingHandle = (uintptr_t) mh;

    LPVOID mapping = MapViewOfFile(mh, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, sizeof *data);
    if (mapping == NULL) {
        CloseHandle(mh);
        CloseHandle(fh);
        LOG(("FLASH: Can't map view of file '%s' (%08x)\n",
            filename, (unsigned)GetLastError()));
        return false;
    }

#else

    int fh = open(filename, O_RDWR | O_CREAT, 0777);
    struct stat st;

    if (fh < 0 || fstat(fh, &st)) {
        if (fh >= 0)
            close(fh);
        LOG(("FLASH: Can't open backing file '%s' (%s)\n",
            filename, strerror(errno)));
        return false;
    }
    fileHandle = fh;

    bool newFile = (unsigned)st.st_size == (unsigned)0;
    if ((unsigned)st.st_size < (unsigned)sizeof *data && ftruncate(fileHandle, sizeof *data)) {
        close(fileHandle);
        LOG(("FLASH: Can't resize backing file '%s' (%s)\n",
            filename, strerror(errno)));
        return false;
    }

    void *mapping = mmap(NULL, sizeof *data, PROT_READ | PROT_WRITE, MAP_SHARED, fileHandle, 0);
    if (mapping == MAP_FAILED) {
        close(fileHandle);
        LOG(("FLASH: Can't memory-map backing file '%s' (%s)\n",
            filename, strerror(errno)));
        return false;
    }

#endif

    data = (FileRecord*) mapping;
    if (newFile)
        initData();

    return true;
}

void FlashStorage::unmapFile()
{
#ifdef _WIN32

    FlushViewOfFile(data, sizeof *data);
    UnmapViewOfFile(data);
    CloseHandle((HANDLE) mappingHandle);
    CloseHandle((HANDLE) fileHandle);

#else

    fsync(fileHandle);
    munmap(data, sizeof *data);
    close(fileHandle);

#endif
}
