/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmruntime.h"
#include "svmloader.h"
#include "elfprogram.h"
#include "flash_blockcache.h"
#include "flash_volume.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmdebugpipe.h"
#include "radio.h"
#include "tasks.h"
#include "panic.h"
#include "cubeslots.h"
#include "cube.h"
#include "panic.h"
#include "audiomixer.h"

#include <stdlib.h>
#include <sifteo/abi.h>

using namespace Svm;

FlashBlockRef SvmLoader::mapRefs[SvmMemory::NUM_FLASH_SEGMENTS];
FlashVolume SvmLoader::mapVols[SvmMemory::NUM_FLASH_SEGMENTS];
uint8_t SvmLoader::runLevel;


void SvmLoader::loadRWData(const Elf::Program &program)
{
    FlashBlockRef ref;
    const Elf::ProgramHeader *ph = program.getRWDataSegment(ref);
    if (!ph)
        return;

    /*
     * Temporarily use segment 0 to do the copy.
     * If anything is wrong, this will cause an SVM fault, just as if
     * the copy was done from userspace.
     */
    SvmMemory::setFlashSegment(0, program.getProgramSpan());
    _SYS_memcpy8(reinterpret_cast<uint8_t*>( ph->p_vaddr ),
                 reinterpret_cast<uint8_t*>( ph->p_offset + SvmMemory::SEGMENT_0_VA ),
                 ph->p_memsz);
}

void SvmLoader::prepareToExec(const Elf::Program &program, SvmRuntime::StackInfo &stack)
{
    // Detach any existing video buffers.
    for (unsigned i = 0; i < _SYS_NUM_CUBE_SLOTS; i++) {
        CubeSlots::instances[i].setVideoBuffer(0);
    }

    // Reset any audio left playing by the previous tenant
    AudioMixer::instance.init();

    // Reset the debugging and logging subsystem
    SvmDebugPipe::init();

    // On simulation, with the built-in debugger, point SvmDebug to
    // the proper ELF binary to load debug symbols from.
    SvmDebugPipe::setSymbolSource(program);

    // Initialize memory
    SvmMemory::erase();
    secondaryUnmap();

    // Load RWDATA into RAM
    loadRWData(program);
 
    // Set up default flash segment
    SvmMemory::setFlashSegment(0, program.getRODataSpan());

    // Init stack
    stack.limit = program.getTopOfRAM();
    stack.top = SvmMemory::VIRTUAL_RAM_TOP;
}

FlashVolume SvmLoader::findLauncher()
{
    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    do {
        if (!vi.next(vol))
            SvmRuntime::fault(F_NO_LAUNCHER);
    } while (vol.getType() != FlashVolume::T_LAUNCHER);

    return vol;
}

void SvmLoader::runLauncher()
{
    FlashVolume vol = findLauncher();
    Elf::Program program;
    if (!program.init(vol.getPayload(mapRefs[0]))) {
        SvmRuntime::fault(F_BAD_ELF_HEADER);
        return;
    }

    runLevel = RUNLEVEL_LAUNCHER;
    mapVols[0] = vol;

    SvmRuntime::StackInfo stack;
    prepareToExec(program, stack);
    SvmRuntime::run(program.getEntry(), stack);
}

void SvmLoader::exec(FlashVolume vol, RunLevel level)
{
    Elf::Program program;
    if (!program.init(vol.getPayload(mapRefs[0]))) {
        SvmRuntime::fault(F_BAD_ELF_HEADER);
        return;
    }

    runLevel = level;
    mapVols[0] = vol;

    SvmRuntime::StackInfo stack;
    prepareToExec(program, stack);
    SvmRuntime::exec(program.getEntry(), stack);
}

FlashMapSpan SvmLoader::secondaryMap(FlashVolume vol)
{
    mapVols[1] = vol;
    FlashMapSpan span = vol.getPayload(mapRefs[1]);
    SvmMemory::setFlashSegment(1, span);
    return span;
}

void SvmLoader::secondaryUnmap()
{
    SvmMemory::setFlashSegment(1, FlashMapSpan::empty());
    mapRefs[1].release();
    mapVols[1].block.setInvalid();
}

FlashVolume SvmLoader::volumeForVA(SvmMemory::VirtAddr va)
{
    unsigned seg = SvmMemory::virtToFlashSegment(va);
    if (seg < arraysize(mapVols))
        return mapVols[seg];
    else
        return FlashMapBlock::invalid();
}

void SvmLoader::exit(bool fault)
{
    switch (runLevel) {

    default:
    case RUNLEVEL_EXEC:
        // Back to the launcher
        exec(findLauncher(), RUNLEVEL_LAUNCHER);
        break;

    case RUNLEVEL_LAUNCHER:
        /*
         * Launcher exited! Normally this doesn't happen. In a production
         * version of the launcher this would be a fatal error. In
         * simulation, however, we use this to exit the simulator.
         */
        #ifdef SIFTEO_SIMULATOR
        // Must preserve the error code here, so that unit tests and other scripts work.
        ::exit(fault);
        #endif
        PanicMessenger::haltForever();
    }
}
