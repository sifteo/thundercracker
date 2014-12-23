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

/*
 * Syscalls for miscellaneous operations which globally effect the runtime
 * execution environment.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmdebugpipe.h"
#include "svmdebugger.h"
#include "svmruntime.h"
#include "svmloader.h"
#include "svmclock.h"
#include "radio.h"
#include "cubeslots.h"
#include "event.h"
#include "tasks.h"
#include "shutdown.h"
#include "idletimeout.h"
#include "ui_pause.h"
#include "ui_coordinator.h"
#include "ui_shutdown.h"
#include "sysinfo.h"

extern "C" {

uint32_t _SYS_getFeatures()
{
    return _SYS_FEATURE_ALL;
}

void _SYS_abort()
{
    SvmRuntime::fault(Svm::F_ABORT);
}

void _SYS_exit(void)
{
    SvmDebugger::signal(Svm::Debugger::S_TERM);
    SvmLoader::exit();
}

void _SYS_yield(void)
{
    Tasks::idle();
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_paint(void)
{
    CubeSlots::paintCubes(CubeSlots::userConnected);
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_paintUnlimited(void)
{
    CubeSlots::paintCubes(CubeSlots::userConnected, false);
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_finish(void)
{
    CubeSlots::finishCubes(CubeSlots::userConnected);
    // Intentionally does _not_ dispatch events!
}

int64_t _SYS_ticks_ns(void)
{
    return SvmClock::ticks();
}

void _SYS_setVector(_SYSVectorID vid, void *handler, void *context)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::setVector(vid, handler, context);

    SvmRuntime::fault(F_SYSCALL_PARAM);
}

void *_SYS_getVectorHandler(_SYSVectorID vid)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::getVectorHandler(vid);

    SvmRuntime::fault(F_SYSCALL_PARAM);
    return NULL;
}

void *_SYS_getVectorContext(_SYSVectorID vid)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::getVectorContext(vid);

    SvmRuntime::fault(F_SYSCALL_PARAM);
    return NULL;
}

void _SYS_setGameMenuLabel(const char *label)
{
    if (label) {
        if (!UIPause::setGameMenuLabel(reinterpret_cast<SvmMemory::VirtAddr>(label)))
            SvmRuntime::fault(F_SYSCALL_ADDRESS);
    } else {
        UIPause::disableGameMenu();
    }
}

void _SYS_setPauseMenuResumeEnabled(bool enabled)
{
    UIPause::setResumeEnabled(enabled);
}

void _SYS_shutdown(uint32_t flags)
{
    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)  |
        Intrinsic::LZ(Tasks::Pause);

    SvmClock::pause();

    if (flags & _SYS_SHUTDOWN_WITH_UI) {
        UICoordinator uic(excludedTasks);
        UIShutdown uiShutdown(uic);
        return uiShutdown.mainLoop();
    }

    ShutdownManager s(excludedTasks);
    return s.shutdown();
}

void _SYS_keepAwake()
{
    IdleTimeout::reset();
}

void _SYS_log(uint32_t t, uintptr_t v1, uintptr_t v2, uintptr_t v3,
    uintptr_t v4, uintptr_t v5, uintptr_t v6, uintptr_t v7)
{
    SvmLogTag tag(t);
    uint32_t type = tag.getType();
    uint32_t arity = tag.getArity();

    v1 = SvmMemory::squashPhysicalAddr(v1);
    v2 = SvmMemory::squashPhysicalAddr(v2);
    v3 = SvmMemory::squashPhysicalAddr(v3);
    v4 = SvmMemory::squashPhysicalAddr(v4);
    v5 = SvmMemory::squashPhysicalAddr(v5);
    v6 = SvmMemory::squashPhysicalAddr(v6);
    v7 = SvmMemory::squashPhysicalAddr(v7);

    switch (type) {

        // Stow all arguments, plus the log tag. The post-processor
        // will do some printf()-like formatting on the stored arguments.
        case _SYS_LOGTYPE_FMT: {
            uint32_t *buffer = SvmDebugPipe::logReserve(tag);
            switch (arity) {
                case 7: buffer[6] = v7;
                case 6: buffer[5] = v6;
                case 5: buffer[4] = v5;
                case 4: buffer[3] = v4;
                case 3: buffer[2] = v3;
                case 2: buffer[1] = v2;
                case 1: buffer[0] = v1;
                case 0:
                default: break;
            }
            SvmDebugPipe::logCommit(tag, buffer, arity * sizeof(uint32_t));
            return;
        }

        // String messages: Only v1 is used (as a pointer), and we emit
        // zero or more log records until we hit the NUL terminator or the
        // end of our mapped memory.
        case _SYS_LOGTYPE_STRING: {
            FlashBlockRef ref;
            bool finished = false;
            do {
                uint32_t *buffer = SvmDebugPipe::logReserve(tag);
                uint8_t *byteBuffer = reinterpret_cast<uint8_t*>(buffer);
                uint32_t chunkSize = 0;

                do {
                    uint8_t *p = SvmMemory::peek<uint8_t>(ref, v1++);
                    if (p) {
                        uint8_t byte = *p;
                        if (byte) {
                            byteBuffer[chunkSize++] = byte;
                        } else {
                            finished = true;
                            break;
                        }
                    } else {
                        SvmRuntime::fault(F_LOG_FETCH);
                        return;
                    }
                } while (chunkSize < SvmDebugPipe::LOG_BUFFER_BYTES);

                SvmDebugPipe::logCommit(SvmLogTag(tag, chunkSize), buffer, chunkSize);

            } while (!finished);
            return;
        }

        // Hex dumps: Like strings, v1 is used as a pointer. The inline
        // parameter from 'tag' is the length of the dump, in bytes. If we're
        // dumping more than what fits in a single log record, emit multiple
        // log records.
        case _SYS_LOGTYPE_HEXDUMP: {
            FlashBlockRef ref;
            uint32_t remaining = tag.getParam();
            while (remaining) {
                uint32_t chunkSize = MIN(SvmDebugPipe::LOG_BUFFER_BYTES, remaining);
                uint32_t *buffer = SvmDebugPipe::logReserve(tag);
                if (!SvmMemory::copyROData(ref, (SvmMemory::PhysAddr)buffer,
                                    (SvmMemory::VirtAddr)v1, chunkSize)) {
                    SvmRuntime::fault(F_LOG_FETCH);
                    return;
                }
                SvmDebugPipe::logCommit(SvmLogTag(tag, chunkSize), buffer, chunkSize);
                remaining -= chunkSize;
                v1 += chunkSize;
            }
            return;
        }

        // Tags that take no parameters
        case _SYS_LOGTYPE_SCRIPT: {
            SvmDebugPipe::logCommit(tag, SvmDebugPipe::logReserve(tag), 0);
            return;
        }

        default:
            ASSERT(0 && "Unknown _SYS_log() tag type");
            return;
    }
}

uint32_t _SYS_version()
{
    /*
     * Return a numeric value that specifies both OS version and hardware version.
     *
     * HW version is encoded in the high byte, and the OS version is encoded
     * in the lower 3 bytes.
     *
     * NOTE: OS_VERSION is currently derived from the git version, and defined
     *       at compile time - see Makefile.defs.
     */

    return (SysInfo::HardwareRev << _SYS_HW_VERSION_SHIFT) | (OS_VERSION & _SYS_OS_VERSION_MASK);
}

}  // extern "C"
