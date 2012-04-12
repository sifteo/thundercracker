/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svm.h"
#include "SvmMemory.h"

using namespace Svm;

uint8_t SvmMemory::userRAM[RAM_SIZE_IN_BYTES] __attribute__ ((aligned(4)));
FlashRange SvmMemory::flashSeg;


bool SvmMemory::mapRAM(VirtAddr va, uint32_t length, PhysAddr &pa)
{
    reg_t offset;
    
    if ((offset = reinterpret_cast<PhysAddr>(va) - userRAM) <= RAM_SIZE_IN_BYTES) {
        // Already a valid PA. This check is required for handling addresses
        // that result from pointer arithmetic on SP.

        pa = reinterpret_cast<PhysAddr>(va);

    } else if ((offset = (uint32_t)va - VIRTUAL_RAM_BASE) <= RAM_SIZE_IN_BYTES) {
        // Standard RAM address virtual-to-physical translation.
        // Note that 'va' may have junk in the upper 32 bits on 64-bit hosts,
        // due to overflow from 32-bit subtraction operations that have been emulated
        // in 64-bit registers.

        pa = userRAM + offset;

    } else {
        // Bad pointer
        return false;
    }

    // Check the extent of this region.
    // Note that with length==0, the address (VIRTUAL_RAM_BASE + RAM_SIZE_IN_BYTES) is valid.
    // This calculation must work securely for any possible 32-bit length value.
    return length <= (RAM_SIZE_IN_BYTES - offset);
}

bool SvmMemory::checkROData(VirtAddr va, uint32_t length)
{
    if (!(va & VIRTUAL_FLASH_BASE)) {
        // RAM address        
        PhysAddr pa;
        return mapRAM(va, length, pa);

    } else {
        // Flash address
        uint32_t flashOffset = (uint32_t)va & ~VIRTUAL_FLASH_BASE;
        return flashOffset < flashSeg.getSize() && (flashSeg.getSize() - flashOffset) >= length;
    }
}

bool SvmMemory::mapROData(FlashBlockRef &ref, VirtAddr va,
    uint32_t &length, PhysAddr &pa)
{
    if (!(va & VIRTUAL_FLASH_BASE)) {
        // RAM address
        return mapRAM(va, length, pa);
    }

    // Flash address
    uint32_t flashOffset = (uint32_t)va & ~VIRTUAL_FLASH_BASE;
    if (flashOffset >= flashSeg.getSize())
        return false;
    flashOffset += flashSeg.getAddress();

    pa = FlashBlock::getBytes(ref, flashOffset, length);
    return true;
}

bool SvmMemory::preload(VirtAddr va)
{
    if (!(va & VIRTUAL_FLASH_BASE)) {
        // Not flash? Nothing to preload.
        return true;
    }

    uint32_t flashOffset = (uint32_t)va & ~VIRTUAL_FLASH_BASE;
    if (flashOffset >= flashSeg.getSize()) {
        // Bad address
        return false;
    }
    flashOffset += flashSeg.getAddress();

    FlashBlock::preload(flashOffset);
    return true;
}

void SvmMemory::validateBase(FlashBlockRef &ref, VirtAddr va,
    PhysAddr &bro, PhysAddr &brw)
{
    if (!(va & VIRTUAL_FLASH_BASE)) {
        // RAM address

        if (mapRAM(va, 1, bro)) {
            brw = bro;
            return;
        }
        
        brw = bro = 0;
        return;
    }

    // Flash address
    uint32_t flashOffset = (uint32_t)va & ~VIRTUAL_FLASH_BASE;
    if (flashOffset >= flashSeg.getSize()) {
        brw = bro = 0;
        return;
    }
    flashOffset += flashSeg.getAddress();
    
    bro = FlashBlock::getByte(ref, flashOffset);
    brw = 0;
}

bool SvmMemory::mapROCode(FlashBlockRef &ref, VirtAddr va, PhysAddr &pa)
{
    // Callers expect us to ignore the two LSBs. All real branch addresses
    // are 32-bit aligned, and some callers use these bits for special purposes.
    uint32_t flashOffset = (uint32_t)va & 0xfffffc;

    // Bounds check, and map to a physical block address
    if (flashOffset >= flashSeg.getSize())
        return false;
    flashOffset += flashSeg.getAddress();

    uint32_t blockOffset = flashOffset & FlashBlock::BLOCK_MASK;
    FlashBlock::get(ref, flashOffset & ~FlashBlock::BLOCK_MASK);
    if (!ref->isCodeOffsetValid(blockOffset))
        return false;

    pa = ref->getData() + blockOffset;
    return true;
}

bool SvmMemory::copyROData(FlashBlockRef &ref,
    PhysAddr dest, VirtAddr src, uint32_t length)
{
    SvmMemory::PhysAddr srcPA;

    while (length) {
        uint32_t chunk = length;
        if (!SvmMemory::mapROData(ref, src, chunk, srcPA))
            return false;

        memcpy(dest, srcPA, chunk);
        dest += chunk;
        src += chunk;
        length -= chunk;
    }
    
    return true;
}

bool SvmMemory::initFlashStream(VirtAddr va, uint32_t length, FlashStream &out)
{
    if (!(va & VIRTUAL_FLASH_BASE)) {
        return false;
    }

    uint32_t flashOffset = (uint32_t)va & ~VIRTUAL_FLASH_BASE;
    if (flashOffset < flashSeg.getSize() && (flashSeg.getSize() - flashOffset) >= length) {
        out.init(flashOffset + flashSeg.getAddress(), length);
        return true;
    }
    
    return false;
}
