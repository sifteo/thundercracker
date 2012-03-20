/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for miscellaneous operations which globally effect the runtime
 * execution environment.
 */

#include <sifteo/abi.h>
#include "svmmemory.h"
#include "svmdebug.h"
#include "svmruntime.h"
#include "radio.h"
#include "cubeslots.h"
#include "event.h"

extern "C" {

void _SYS_abort() {
    SvmDebug::fault(Svm::F_ABORT);
}

void _SYS_exit(void)
{
    SvmRuntime::exit();
}

void _SYS_yield(void)
{
    Radio::halt();
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_paint(void)
{
    CubeSlots::paintCubes(CubeSlots::vecEnabled);
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_finish(void)
{
    CubeSlots::finishCubes(CubeSlots::vecEnabled);
    SvmRuntime::dispatchEventsOnReturn();
}

int64_t _SYS_ticks_ns(void)
{
    int64_t ns = SysTime::ticks();
    ASSERT(ns > 0);
    return ns;
}

void _SYS_setVector(_SYSVectorID vid, void *handler, void *context)
{
    if (vid < _SYS_NUM_VECTORS)
        Event::setVector(vid, handler, context);
}

void *_SYS_getVectorHandler(_SYSVectorID vid)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::getVectorHandler(vid);
    return NULL;
}

void *_SYS_getVectorContext(_SYSVectorID vid)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::getVectorContext(vid);
    return NULL;
}

void _SYS_log(uint32_t t, uintptr_t v1, uintptr_t v2, uintptr_t v3,
    uintptr_t v4, uintptr_t v5, uintptr_t v6, uintptr_t v7)
{
    SvmLogTag tag(t);
    uint32_t type = tag.getType();
    uint32_t arity = tag.getArity();

    SvmMemory::squashPhysicalAddr(v1);
    SvmMemory::squashPhysicalAddr(v2);
    SvmMemory::squashPhysicalAddr(v3);
    SvmMemory::squashPhysicalAddr(v4);
    SvmMemory::squashPhysicalAddr(v5);
    SvmMemory::squashPhysicalAddr(v6);
    SvmMemory::squashPhysicalAddr(v7);

    switch (type) {

        // Stow all arguments, plus the log tag. The post-processor
        // will do some printf()-like formatting on the stored arguments.
        case _SYS_LOGTYPE_FMT: {
            uint32_t *buffer = SvmDebug::logReserve(tag);        
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
            SvmDebug::logCommit(tag, buffer, arity * sizeof(uint32_t));
            return;
        }

        // String messages: Only v1 is used (as a pointer), and we emit
        // zero or more log records until we hit the NUL terminator.
        case _SYS_LOGTYPE_STRING: {
            const unsigned chunkSize = SvmDebug::LOG_BUFFER_BYTES;
            FlashBlockRef ref;
            for (;;) {
                uint32_t *buffer = SvmDebug::logReserve(tag);
                if (!SvmMemory::copyROData(ref, (SvmMemory::PhysAddr)buffer,
                                    (SvmMemory::VirtAddr)v1, chunkSize)) {
                    SvmDebug::fault(F_LOG_FETCH);
                    return;
                }

                // strnlen is not easily available on mingw...
                const char *str = reinterpret_cast<const char*>(buffer);
                const char *end = static_cast<const char*>(memchr(str, '\0', chunkSize));
                uint32_t bytes = end ? (size_t) (end - str) : chunkSize;

                SvmDebug::logCommit(SvmLogTag(tag, bytes), buffer, bytes);
                if (bytes < chunkSize)
                    return;
            }
        }

        // Hex dumps: Like strings, v1 is used as a pointer. The inline
        // parameter from 'tag' is the length of the dump, in bytes. If we're
        // dumping more than what fits in a single log record, emit multiple
        // log records.
        case _SYS_LOGTYPE_HEXDUMP: {
            FlashBlockRef ref;
            uint32_t remaining = tag.getParam();
            while (remaining) {
                uint32_t chunkSize = MIN(SvmDebug::LOG_BUFFER_BYTES, remaining);
                uint32_t *buffer = SvmDebug::logReserve(tag);
                if (!SvmMemory::copyROData(ref, (SvmMemory::PhysAddr)buffer,
                                    (SvmMemory::VirtAddr)v1, chunkSize)) {
                    SvmDebug::fault(F_LOG_FETCH);
                    return;
                }
                SvmDebug::logCommit(SvmLogTag(tag, chunkSize), buffer, chunkSize);
                remaining -= chunkSize;
            }
            return;
        }

        default:
            ASSERT(0 && "Unknown _SYS_log() tag type");
            return;
    }
}

}  // extern "C"
