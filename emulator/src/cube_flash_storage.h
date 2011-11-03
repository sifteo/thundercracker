/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CUBE_FLASH_STORAGE_H
#define _CUBE_FLASH_STORAGE_H

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <io.h>
#define ftruncate _chsize
#else
#include <unistd.h>
#endif

#include "vtime.h"
#include "cube_cpu.h"
#include "cube_flash_model.h"

namespace Cube {


/*
 * Memory/disk backing for nonvolatile memory
 */
 
class FlashStorage {
 public:
    struct {
        uint8_t nvm[1024];                  // Nordic nonvolatile memory
        uint8_t ext[FlashModel::SIZE];      // External NOR flash
    } data;

    bool init(const char *filename=NULL);
    void exit();
    
    void asyncWrite(const VirtualTime &vtime, CPU::em8051 *cpu) {
        // Schedule a flush to disk at some future time.
        cpu->needHardwareTick = true;
        write_timer = vtime.clocks + vtime.msec(FLUSH_TIME_MS);
    }

    void asyncWrite(TickDeadline &deadline) {
        // Schedule a flush to disk at some future time.
        write_timer = deadline.setRelative(VirtualTime::msec(FLUSH_TIME_MS));
    }

    ALWAYS_INLINE void tick(TickDeadline &deadline) {
        if (UNLIKELY(write_timer)) {
            if (deadline.hasPassed(write_timer)) {
                write();
                write_timer = 0;
            } else {
                deadline.set(write_timer);
            }
        }
    }

 private:
    void write();

    /*
     * How long (in microseconds of virtual time) to wait for
     * writes/erases to cease before we rewrite the flash file on disk.
     *
     * This is a simple way for us to write the flash to disk in a
     * non-error-prone way while also avoiding a lot of unnecessary
     * write() calls, or platform-specific memory mapping.
     */
    static const unsigned FLUSH_TIME_MS = 500;

    uint64_t write_timer;
    FILE *file;
};


};  // namespace Cube

#endif
