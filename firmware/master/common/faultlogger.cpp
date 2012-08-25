/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "faultlogger.h"
#include "svm.h"
#include "svmloader.h"
#include "cubeslots.h"
#include "led.h"
#include "tasks.h"
#include "ui_fault.h"
#include "svmmemory.h"

#ifndef SIFTEO_SIMULATOR
#   include "powermanager.h"
#endif

SysLFS::FaultHeader FaultLogger::header;
SysLFS::FaultCubeInfo FaultLogger::cubes;
SysLFS::FaultRegisters FaultLogger::regs;
FlashBlockRef FaultLogger::codePage;


void FaultLogger::internalError(unsigned code)
{
    // Set up panic LED state very early on
    LED::set(LEDPatterns::panic, true);

    // Will anyone hear us?
    LOG(("FAULT: Internal error %08x\n", code));
    UART("FAULT: Internal error ");
    UART_HEX(code);
    UART("\r\n");

    ASSERT(0 && "Internal error");

    // Turn power off if we can
    #ifndef SIFTEO_SIMULATOR
    PowerManager::batteryPowerOff();
    #endif

    while (1) {}
}

void FaultLogger::reportSvmFault(unsigned code)
{
    UART("FAULT: SVM ");
    UART_HEX(code);
    UART("\r\n");

    // Set up panic LED state very early on
    LED::set(LEDPatterns::panic, true);

    // Ignore double-faults (usually abort traps)
    if (Tasks::isPending(Tasks::FaultLogger))
        return;

    fillHeader(code, SysLFS::kFaultSVM);
    fillCubes();
    fillRegs();
    captureCodePage();

    Tasks::trigger(Tasks::FaultLogger);

    UART("PC = "); UART_HEX(regs.pc);
    UART("\r\nSP = "); UART_HEX(regs.sp);
    UART("\r\nFP = "); UART_HEX(regs.fp);
    UART("\r\nGPR =");
    for (unsigned i = 0; i < 8; ++i) {
        UART(" ");
        UART_HEX(regs.gpr[i]);
    }
    UART("\r\n");
}

void FaultLogger::fillHeader(unsigned code, unsigned type)
{
    header.reference = 0;       // Placeholder
    header.recordType = type;
    header.runningVolume = SvmLoader::getRunningVolume().block.code;
    header.code = code;
    header.uptime = SysTime::ticks();
}

void FaultLogger::fillCubes()
{
    cubes.sysConnected = CubeSlots::sysConnected;
    cubes.userConnected = CubeSlots::userConnected;
    cubes.minUserCubes = CubeSlots::minUserCubes;
    cubes.maxUserCubes = CubeSlots::maxUserCubes;
    cubes.reserved = 0;
}

void FaultLogger::fillRegs()
{
    regs.pc = SvmRuntime::reconstructCodeAddr(SvmCpu::reg(REG_PC));
    regs.sp = SvmMemory::squashPhysicalAddr(SvmCpu::reg(REG_SP));
    regs.fp = SvmMemory::squashPhysicalAddr(SvmCpu::reg(REG_FP));
    regs.gpr[0] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(0));
    regs.gpr[1] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(1));
    regs.gpr[2] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(2));
    regs.gpr[3] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(3));
    regs.gpr[4] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(4));
    regs.gpr[5] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(5));
    regs.gpr[6] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(6));
    regs.gpr[7] = SvmMemory::squashPhysicalAddr(SvmCpu::reg(7));
}

void FaultLogger::captureCodePage()
{
    /*
     * Allocate an anonymous page, copy our real code page to it,
     * then issue an abort trap to that page so that we can capture
     * the SVM control flow.
     */

    FlashBlock *original = SvmRuntime::getCodeBlock();
    if (original) {
        original->invalidateBlock(FlashBlock::F_ABORT_TRAP);
        FlashBlock::anonymous(codePage);
        memcpy(codePage->getData(), original->getData(), FlashBlock::BLOCK_SIZE);
    }
}

unsigned FaultLogger::buildFaultRecord(uint8_t *buffer)
{
    switch (header.recordType) {

    case SysLFS::kFaultSVM:
        buildFaultRecord((SysLFS::FaultRecordSvm*) buffer);
        return sizeof(SysLFS::FaultRecordSvm);

    // Header only
    default:
        memcpy(buffer, &header, sizeof header);
        return sizeof header;
    }
}

void FaultLogger::buildFaultRecord(SysLFS::FaultRecordSvm *buffer)
{
    memset(buffer, SENTINEL, sizeof *buffer);

    buffer->header = header;
    buffer->cubes = cubes;
    buffer->regs = regs;

    // Save the top of the stack
    safeMemoryDump(buffer->mem.stack, buffer->regs.sp, sizeof buffer->mem.stack);

    // Save the code page (which we stashed in anonyous memory earlier)
    if (codePage.isHeld()) {
        memcpy(buffer->mem.codePage, codePage->getData(), FlashBlock::BLOCK_SIZE);
        codePage.release();
    }

    // Save volume metadata
    FlashVolume vol = FlashMapBlock::fromCode(header.runningVolume);
    if (vol.isValid()) {
        FlashBlockRef mapRef;
        Elf::Program program;
        if (program.init(vol.getPayload(mapRef))) {
            dumpMetadata(program, _SYS_METADATA_UUID, &buffer->vol.uuid, sizeof buffer->vol.uuid);
            dumpMetadata(program, _SYS_METADATA_PACKAGE_STR, &buffer->vol.package, sizeof buffer->vol.package);
            dumpMetadata(program, _SYS_METADATA_VERSION_STR, &buffer->vol.version, sizeof buffer->vol.version);
        }
    }
}

void FaultLogger::safeMemoryDump(uint8_t *dest, SvmMemory::VirtAddr src, unsigned length)
{
    FlashBlockRef ref;

    while (length) {
        uint8_t *p = SvmMemory::peek<uint8_t>(ref, src);
        if (!p)
            return;

        *dest = *p;
        length--;
        src++;
        dest++;
    }
}

void FaultLogger::dumpMetadata(const Elf::Program &program, uint16_t key, void *dest, unsigned destSize)
{
    FlashBlockRef ref;
    uint32_t actualSize;
    const void *src = program.getMeta(ref, key, 1, actualSize);
    if (src)
        memcpy(dest, src, MIN(destSize, actualSize));
}

void FaultLogger::task()
{
    LOG(("FAULT: Logger task entered\n"));
    UART("FAULT: Logger task entered\r\n");

    /*
     * Read the most recent fault, so we know where to store this one.
     * If any faults have been logged already, our serial number and key
     * are subsequent to the last fault.
     */

    SysLFS::FaultHeader lastHeader;
    SysLFS::Key lastKey = lastHeader.readLatest();
    SysLFS::Key key;

    if (lastKey == SysLFS::kEnd) {
        // First fault in this filesystem
        key = SysLFS::kFaultBase;
        header.reference = 1;
    } else {
        key = SysLFS::FaultHeader::nextKey(lastKey);
        header.reference = lastHeader.reference + 1;
    }

    /*
     * Use the bottom of userspace RAM as temporary space to build our
     * fault record. The actual composition of the record is type-specific.
     */

    SvmMemory::VirtAddr recordVA = SvmMemory::VIRTUAL_RAM_BASE;
    SvmMemory::PhysAddr recordPA;
    SvmMemory::mapRAM(recordVA, FlashLFSIndexRecord::MAX_SIZE, recordPA);

    unsigned length = buildFaultRecord(recordPA);

    ASSERT(length <= FlashLFSIndexRecord::MAX_SIZE);
    SysLFS::write(key, recordPA, length, true);

    /*
     * Now display the fault UI, until the user dismisses it.
     */

    const uint32_t excludedTasks =
        Intrinsic::LZ(Tasks::AudioPull)   |
        Intrinsic::LZ(Tasks::FaultLogger) |
        Intrinsic::LZ(Tasks::Pause);

    UICoordinator uic(excludedTasks);
    UIFault uiFault(uic, header.reference);
    uiFault.mainLoop();

    /*
     * Done logging fault info.
     *
     * Exit this SVM process. Currently this always causes us to return
     * to the launcher, on hardware, once we resume running userspace code.
     * On simulation, it exits Siftulator immediately.
     */

    SvmLoader::exit(true);
}
