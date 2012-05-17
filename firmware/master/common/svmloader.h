/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_LOADER_H
#define SVM_LOADER_H

#include "macros.h"
#include "elfprogram.h"
#include "flash_volume.h"
#include "svmmemory.h"


class SvmLoader {
public:
    SvmLoader();  // Do not implement

    // Run the default program. Never exits.
    static void runDefault();

    // Load a program into the primary flash segment, and run it
    static void run(const Elf::Program &program);
    static void run(FlashVolume vol);

    // Map a full volume in the secondary flash segment.
    static void map(FlashVolume vol);

    static void exit(bool fault=false);

private:
    static FlashBlockRef mapRefs[SvmMemory::NUM_FLASH_SEGMENTS];

    static _SYSCubeIDVector getCubeVector(const Elf::Program &program);
    static void bootstrap(const Elf::Program &program);
    static void bootstrapAssets(const Elf::Program &program, _SYSCubeIDVector cubes);

    static void logTitleInfo(const Elf::Program &program);
    static void loadRWData(const Elf::Program &program);
};


#endif // SVM_LOADER_H
