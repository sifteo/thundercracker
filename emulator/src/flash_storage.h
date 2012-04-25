/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * This is a centralized storage for all simulated flash memory.
 *
 * All of this storage is defined in a fixed-layout structure, which
 * can be backed either by anonymous RAM or by a mapped file.
 */

#ifndef _FLASH_STORAGE_H
#define _FLASH_STORAGE_H

#include <stdint.h>
#include <stdio.h>
#include <sifteo/abi.h>
#include "cube_flash_model.h"
#include "flash_device.h"


class FlashStorage {
 public:

    struct CubeRecord {
        uint8_t nvm[1024];                      // Nordic nonvolatile memory
        uint8_t ext[Cube::FlashModel::SIZE];    // External NOR flash
    };

    struct MasterRecord {
        uint8_t bytes[FlashDevice::CAPACITY];
    };

    struct HeaderRecord {
        union {
            uint8_t bytes[FlashDevice::PAGE_SIZE];    // Pad to one full page

            struct {
                uint64_t    magic;
                uint32_t    version;
                uint32_t    fileSize;
                uint32_t    cube_count;
                uint32_t    cube_nvmSize;
                uint32_t    cube_extSize;
                uint32_t    mc_pageSize;
                uint32_t    mc_capacity;
            };
        };

        static const uint64_t MAGIC             = 0x534c467974666953LLU;
        static const uint32_t CURRENT_VERSION   = 1;
    };

    struct FileRecord {
        HeaderRecord   header;
        MasterRecord   master;
        CubeRecord     cubes[_SYS_NUM_CUBE_SLOTS];
    };

    FileRecord *data;

    FlashStorage();
    ~FlashStorage();

    bool init(const char *filename=NULL);
    void exit();

 private:
    bool isInitialized;
    bool isFileBacked;
    uintptr_t fileHandle;
    uintptr_t mappingHandle;

    bool mapFile(const char *filename);
    void unmapFile();

    void initData();
    bool checkData();
};


#endif
