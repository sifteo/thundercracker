/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MEMORY_H
#define SVM_MEMORY_H

#include <stdint.h>
#include <inttypes.h>

#include "svm.h"
#include "flashlayer.h"


class SvmMemory {
public:
    typedef uint8_t* PhysAddr;
    typedef Svm::reg_t VirtAddr;

    /**
     * RAM-only, validate a range of bytes and return a physical base
     * pointer. Always fails on flash addresses. Requested memory is always
     * contiguous and fully mapped. Returns 'true' iff va was successfully
     * mapped. Returns 'false' if 'va' is any invalid address, including NULL.
     */
    static bool mapRAM(VirtAddr va, uint32_t length, PhysAddr &pa);

    /**
     * Convenience wrapper for RAM-only validation of arbitrary pointer types.
     * If 'allowNULL' is true and 'ptr' is 0, we return true without changing
     * 'ptr'. In all other cases of invalid virtual address, we return false.
     */
    template <typename T>
    static inline bool mapRAM(T &ptr, uint32_t length, bool allowNULL=false)
    {
        if (allowNULL && !ptr)
            return true;
        
        VirtAddr va = reinterpret_cast<VirtAddr>(ptr);
        PhysAddr pa;

        if (!mapRAM(va, length, pa))
            return false;
        
        ptr = reinterpret_cast<T>(pa);
        return true;
    }
    
    /**
     * Overflow-safe array size calculation. Saturates instead of overflowing.
     */
    static inline uint32_t arraySize(uint32_t itemSize, uint32_t count)
    {
        // In 16x16-bit multiply, 32-bit overflow cannot occur.
        STATIC_ASSERT(RAM_SIZE_IN_BYTES <= 0x10000);

        if (itemSize < RAM_SIZE_IN_BYTES && count < RAM_SIZE_IN_BYTES)
            return itemSize * count;
        else
            return 0xFFFFFFFFU;
    }
    
    /**
     * Read-only data memory validator. Handles RAM or Flash addresses.
     *
     * Even if the entire region is valid, it is not guaranteed to all be
     * available for use. The actual number of available bytes is returned
     * via the 'length' parameter. Additional data beyond this number
     * of bytes may require a separate validation.
     *
     * Since this is used by clients who are expected to handle flash addresses,
     * a FlashBlockRef must be supplied in order to receive a cache block reference
     * in the event that we need to take one.
     *
     * If this returns 'true' (success), 'length' is guaranteed to be >= 1.
     */
    static bool mapROData(FlashBlockRef &ref, VirtAddr va,
        uint32_t &length, PhysAddr &pa);

    /**
     * Check whether a Read-only Data region is valid in our virtual address
     * space, without actually mapping it.
     */
    static bool checkROData(VirtAddr va, uint32_t length);

    /**
     * Read-only code memory validator. Ensures that the supplied va is a
     * legal jump target, and maps it to a physical address.
     *
     * Assumes 'va' is a flash address. The VIRTUAL_FLASH_BASE bit is ignored.
     */
    static bool mapROCode(FlashBlockRef &ref, VirtAddr va, PhysAddr &pa);

    /**
     * Copy out read-only data into arbitrary physical RAM. This is a more
     * convenient alternative for small data structures that are not necessarily
     * block aligned.
     */
    static bool copyROData(FlashBlockRef &ref, PhysAddr dest, VirtAddr src, uint32_t length);

    /**
     * Initialize a FlashStream with read-only data from a flash pointer.
     * Does NOT support RAM addresses, unlike all the *ROData functions.
     */
    static bool initFlashStream(VirtAddr va, uint32_t length, FlashStream &out);

    /**
     * Convenient type-safe wrapper around copyROData.
     */
    template <typename T>
    static inline bool copyROData(T &dest, const T *src)
    {
        FlashBlockRef ref;
        return copyROData(ref, reinterpret_cast<PhysAddr>(&dest),
                          reinterpret_cast<VirtAddr>(src), sizeof(T));
    }

    /**
     * Convenience functions to read a single value from RAM or Flash.
     * On error, returns a default value.
     */
    template <typename T>
    static inline T peek(FlashBlockRef &ref, VirtAddr va, T defaultValue=0) {
        uint32_t length = sizeof(T);
        PhysAddr pa;
        
        if (mapROData(ref, va, length, pa) && length == sizeof(T))
            return *reinterpret_cast<T*>(pa);
        else
            return defaultValue;
    }

    /**
     * Set the bounds of the flash memory region reachable through SVM.
     * This is initialized to the current binary's RODATA segment by our ELF
     * loader.
     */
    static void setFlashSegment(uint32_t base, uint32_t size) {
        ASSERT((base & FlashBlock::BLOCK_MASK) == 0);
        ASSERT((size & FlashBlock::BLOCK_MASK) == 0);
        
        // Masks here are for security and reliability in case of a
        // badly constructed ELF file.
        flashBase = base & ~FlashBlock::BLOCK_MASK;
        flashSize = size & ~FlashBlock::BLOCK_MASK;
    }

private:
    static const unsigned VIRTUAL_FLASH_BASE = 0x80000000;
    static const unsigned VIRTUAL_RAM_BASE = 0x10000;
    static const unsigned RAM_SIZE_IN_BYTES = 32 * 1024;

    static uint8_t userRAM[RAM_SIZE_IN_BYTES];
    
    static uint32_t flashBase;
    static uint32_t flashSize;
};


class SvmMemStream


#endif // SVM_MEMORY_H
