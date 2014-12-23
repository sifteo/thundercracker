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

#include "svmruntime.h"
#include "svmloader.h"
#include "elfprogram.h"
#include "flash_blockcache.h"
#include "flash_volume.h"
#include "svm.h"
#include "svmmemory.h"
#include "svmfastlz.h"
#include "svmdebugpipe.h"
#include "svmclock.h"
#include "radio.h"
#include "tasks.h"
#include "cubeslots.h"
#include "cube.h"
#include "faultlogger.h"
#include "ui_pause.h"
#include "audiomixer.h"
#include "event.h"
#include "led.h"
#include "assetloader.h"
#include "btprotocol.h"
#include "xmtrackerplayer.h"

#ifdef SIFTEO_SIMULATOR
#   include "system_mc.h"
#endif

#include <stdlib.h>
#include <sifteo/abi.h>

using namespace Svm;

FlashBlockRef SvmLoader::mapRefs[SvmMemory::NUM_FLASH_SEGMENTS];
FlashVolume SvmLoader::mapVols[SvmMemory::NUM_FLASH_SEGMENTS];
FlashVolume SvmLoader::previousVolume;
uint8_t SvmLoader::runLevel;


bool SvmLoader::loadRWData(const Elf::Program &program)
{
    /*
     * The RWDATA segment is the only one wa actually *load*,
     * instead of just mapping or zero'ing. As such, we support
     * unpacking the data in a segment-type-specific way.
     *
     * Returns true on success, false on failure. Does not fault.
     */

    FlashBlockRef ref;
    const Elf::ProgramHeader *ph = program.getRWDataSegment(ref);
    if (!ph) {
        // It's legal to have no RWDATA segment
        return true;
    }

    /*
     * Temporarily use segment 0 to do the copy, so we can use
     * SvmMemory infrastructure to do this.
     */

    SvmMemory::setFlashSegment(0, program.getProgramSpan());
    SvmMemory::VirtAddr srcVA = ph->p_offset + SvmMemory::SEGMENT_0_VA;
    SvmMemory::VirtAddr destVA = ph->p_vaddr;
    SvmMemory::PhysAddr destPA;

    uint32_t destLen = ph->p_memsz;
    uint32_t srcLen = ph->p_filesz;
    if (!SvmMemory::mapRAM(destVA, destLen, destPA))
        return false;

    /*
     * Type-specific loading operation.
     */
    
    switch (ph->p_type) {

        // Uncompressed
        case Elf::PT_LOAD:
            return SvmMemory::copyROData(ref, destPA, srcVA, destLen);

        // Compressed with FastLZ level 1
        case _SYS_ELF_PT_LOAD_FASTLZ:
            return SvmFastLZ::decompressL1(ref, destPA, destLen, srcVA, srcLen);

        default:
            return false;
    }
}

bool SvmLoader::prepareToExec(const Elf::Program &program, SvmRuntime::StackInfo &stack)
{
    // Resync userspace clock with system clock
    SvmClock::init();

    // Default LED behavior
    LED::set(LEDPatterns::idle);

    // Reset all event vectors
    Event::clearVectors();
    UIPause::disableGameMenu();
    UIPause::setResumeEnabled(true);

    // Cancel and detach the AssetLoader if necessary
    AssetLoader::init();

    // Detach any existing cube buffers.
    for (unsigned i = 0; i < _SYS_NUM_CUBE_SLOTS; i++) {
        CubeSlots::instances[i].setVideoBuffer(0);
        CubeSlots::instances[i].setMotionBuffer(0);
    }

    // Reset Bluetooth userspace state
    BTProtocol::setUserQueues(0, 0);
    BTProtocol::setUserState(0, 0);

    if (BTProtocol::isConnected()) {
        BTProtocol::reportVolume();
    }

    // Reset any audio left playing by the previous tenant
    XmTrackerPlayer::instance.stop();
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
    if (!loadRWData(program)) {
        SvmRuntime::fault(F_RWDATA_SEG);
        return false;
    }

    // Set up default flash segment
    SvmMemory::setFlashSegment(0, program.getRODataSpan());

    // Init stack
    stack.limit = program.getTopOfRAM();
    stack.top = SvmMemory::VIRTUAL_RAM_TOP;

    return true;
}

FlashVolume SvmLoader::findLauncher()
{
    FlashVolumeIter vi;
    FlashVolume vol;

    vi.begin();
    while (vi.next(vol)) {
        if (vol.getType() == FlashVolume::T_LAUNCHER)
            return vol;
    }

    // No launcher! Log a fault, then stick in a task loop.

    SvmRuntime::fault(F_NO_LAUNCHER);
    while (1)
        Tasks::idle();
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
    if (prepareToExec(program, stack))
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
    previousVolume = mapVols[0];
    mapVols[0] = vol;

    SvmRuntime::StackInfo stack;
    if (prepareToExec(program, stack)) {
        SvmRuntime::exec(program.getEntry(), stack);
    }
}

void SvmLoader::execLauncher()
{
    exec(findLauncher(), RUNLEVEL_LAUNCHER);
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
        return execLauncher();

    case RUNLEVEL_LAUNCHER:
        // Launcher exited! Shut down.

        #ifdef SIFTEO_SIMULATOR
        // Must preserve the error code here, so that unit tests and other scripts work.
        SystemMC::exit(fault);
        #endif

        return _SYS_shutdown(0);
    }
}
