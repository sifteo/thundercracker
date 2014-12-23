/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware
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

#ifndef SVM_LOADER_H
#define SVM_LOADER_H

#include "macros.h"
#include "elfprogram.h"
#include "flash_map.h"
#include "flash_volume.h"
#include "svmmemory.h"
#include "svmruntime.h"


class SvmLoader {
public:
    enum RunLevel {
        RUNLEVEL_LAUNCHER,
        RUNLEVEL_EXEC,
    };

    SvmLoader();  // Do not implement

    // Run the launcher program. Does not return.
    static void runLauncher();

    // Exit the current program. 
    static void exit(bool fault=false);

    // During program execution, load a new program
    static void exec(FlashVolume vol, RunLevel level = RUNLEVEL_EXEC);

    // Exec the launcher in place of whatever's currently running. Does return.
    static void execLauncher();

    static ALWAYS_INLINE RunLevel getRunLevel() {
        return RunLevel(runLevel);
    }

    /**
     * Map a full volume in the secondary flash segment.
     * Returns a span, whose map point is valid until the next secondaryMap()
     * or secondaryUnmap() operation.
     */
    static FlashMapSpan secondaryMap(FlashVolume vol);
    static void secondaryUnmap();

    // Return the currently executing program's FlashVolume
    static FlashVolume getRunningVolume() {
        return mapVols[0];
    }

    // Return the last running volume (the one that exec()'ed the current one) if any.
    static FlashVolume getPreviousVolume() {
        return previousVolume;
    }

    // Is this volume mapped anywhere?
    static bool isVolumeMapped(FlashVolume vol) {
        STATIC_ASSERT(arraysize(mapVols) == 2);
        return vol.block.code == mapVols[0].block.code ||
               vol.block.code == mapVols[1].block.code;
    }

    // Return the volume corresponding with a particular VA.
    // If the VA isn't a valid flash address, returns an invalid volume.
    static FlashVolume volumeForVA(SvmMemory::VirtAddr va);

private:
    static FlashBlockRef mapRefs[SvmMemory::NUM_FLASH_SEGMENTS];
    static FlashVolume mapVols[SvmMemory::NUM_FLASH_SEGMENTS];
    static uint8_t runLevel;
    static FlashVolume previousVolume;

    static FlashVolume findLauncher();
    static bool prepareToExec(const Elf::Program &program, SvmRuntime::StackInfo &stack);
    static bool loadRWData(const Elf::Program &program);
};


#endif // SVM_LOADER_H
