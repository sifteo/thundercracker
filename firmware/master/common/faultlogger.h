/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _FAULT_LOGGER_H
#define _FAULT_LOGGER_H

#include <sifteo/abi.h>
#include "macros.h"
#include "flash_syslfs.h"
#include "svmmemory.h"
#include "elfprogram.h"

/**
 * This module gives us a way to report a fault from anywhere, then
 * asynchronously log it to SysLFS and deliver a modal message to
 * the user.
 *
 * The 'report*Fault' entry points can be called from any context.
 */

class FaultLogger {
public:
    static const uint8_t SENTINEL = 0xFC;

    enum InternalErrorCodes {
        F_OUT_OF_CACHE_BLOCKS = 1,
        F_SVMCPU_RUN_RETURN,
    };

    // Last-ditch handlers for very low-level failures we don't expect to see
    static void internalError(unsigned code) NORETURN;

    // Report errors to the user and to the log
    static void reportSvmFault(unsigned code);

    // Task worker to log this fault and display a blocking UI.
    static void task();

private:
    FaultLogger();   // Do not implement

    // Runs anywhere
    static void fillHeader(unsigned code, unsigned type);
    static void fillCubes();
    static void fillRegs();
    static void captureCodePage();

    // Runs in task context
    static unsigned buildFaultRecord(uint8_t *buffer);
    static void buildFaultRecord(SysLFS::FaultRecordSvm *buffer);
    static void safeMemoryDump(uint8_t *dest, SvmMemory::VirtAddr src, unsigned length);
    static void dumpMetadata(const Elf::Program &program, uint16_t key, void *dest, unsigned destSize);

    // Info that we can collect immediately
    static SysLFS::FaultHeader header;
    static SysLFS::FaultCubeInfo cubes;
    static SysLFS::FaultRegisters regs;

    // Cache reference for an anonymous page we copy the current SVM code page to
    static FlashBlockRef codePage;
};


#endif
