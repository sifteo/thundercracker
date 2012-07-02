/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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

    static FlashVolume findLauncher();
    static void prepareToExec(const Elf::Program &program, SvmRuntime::StackInfo &stack);
    static void loadRWData(const Elf::Program &program);
};


#endif // SVM_LOADER_H
