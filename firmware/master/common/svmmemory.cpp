/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "svmmemory.h"

uint8_t SvmMemory::userRAM[RAM_SIZE_IN_BYTES] __attribute__ ((align(4)));
uint32_t SvmMemory::flashBase;
uint32_t SvmMemory::flashSize;


bool SvmMemory::mapRAM(VirtAddr va, uint32_t length, PhysAddr &pa)
{
    reg_t offset;
    
    if ((offset = reinterpret_cast<PhysAddr>(va) - userRAM) < RAM_SIZE_IN_BYTES) {
        // Already a valid PA. This check is required for handling addresses
        // that result from pointer arithmetic on SP.
        pa = reinterpret_cast<PhysAddr>(va);

    } else if ((offset = va - VIRTUAL_RAM_BASE) < RAM_SIZE_IN_BYTES) {
        // Standard RAM address virtual-to-physical translation
        pa = userRAM + offset;

    } else {
        // Bad pointer
        return false;
    }

    // Check the extent of this region
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
        return flashOffset < flashSize && (flashSize - flashOffset) >= length;
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
    if (flashOffset >= flashSize)
        return false;
    flashOffset += flashBase;

    pa = FlashBlock::getBytes(ref, flashOffset, length);
    return true;
}

bool SVMMemory::mapROCode(FlashBlockRef &ref, VirtAddr va, PhysAddr &pa)
{
    uint32_t flashOffset = (uint32_t)va & ~VIRTUAL_FLASH_BASE;
    if (flashOffset >= flashSize)
        return false;
    flashOffset += flashBase;

    uint32_t blockOffset = flashOffset & BLOCK_MASK;
    get(ref, flashOffset - blockOffset);
    if (!ref->isCodeOffsetValid(blockOffset))
        return false;

    pa = ref->getData() + blockOffset;
    return true;
}

bool SVMMemory::copyROData(PhysAddr dest, VirtAddr src, uint32_t length)
{
    FlashBlockRef ref;
    SvmMemory::PhysAddr srcPA;

    while (count) {
        uint32_t chunk = count;
        if (!SvmMemory::mapROData(ref, src, chunk, srcPA))
            return;

        memcpy(dest, srcPA, chunk);
        dest += chunk;
        src += chunk;
        count -= chunk;
    }
}
