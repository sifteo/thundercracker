/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_LOADER_H
#define SVM_LOADER_H

#include <stdint.h>
#include <inttypes.h>
#include "elfprogram.h"


class SvmLoader {
public:
    SvmLoader();  // Do not implement

    static void run(const Elf::Program &program);
    static void run(int id);
    static void exit(bool fault=false);

private:
    static _SYSCubeIDVector getCubeVector(const Elf::Program &program);
    static void bootstrap(const Elf::Program &program);
    static void bootstrapAssets(const Elf::Program &program, _SYSCubeIDVector cubes);

    static void logTitleInfo(const Elf::Program &program);
    static void loadRWData(const Elf::Program &program);
};


#endif // SVM_LOADER_H
