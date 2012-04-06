/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MEMORY_H
#define SVM_MEMORY_H

#include <stdint.h>
#include <inttypes.h>
#include <string.h>

#include "svm.h"
#include "flashlayer.h"


class SvmMemory {
public:
    static const unsigned VIRTUAL_FLASH_BASE = 0x80000000;
    static const unsigned VIRTUAL_RAM_BASE = 0x10000;
    static const unsigned RAM_SIZE_IN_BYTES = 32 * 1024;
    static const unsigned VIRTUAL_RAM_TOP = VIRTUAL_RAM_BASE + RAM_SIZE_IN_BYTES;

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
     * Convenience wrapper for RAM-only validation of fixed-size objects.
     * Always fails for NULL pointers.
     */
    template <typename T>
    static inline bool mapRAM(T &ptr)
    {
        return mapRAM(ptr, sizeof *ptr, false);
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
     * Specialized read-only memory validator for internal use by the SvmRuntime.
     * This is similar to mapROData, but it has no length check, and it provides
     * separate read-only and read-write base pointer outputs.
     *
     * If the address is valid for read but not for write, brw will be 0. If
     * the address is totally invalid, both bro and brw will be 0.
     */
    static void validateBase(FlashBlockRef &ref, VirtAddr va,
        PhysAddr &bro, PhysAddr &brw);

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
     * Asynchronously preload the given VirtAddr. If it's a flash address, this
     * may try to page in that flash block ahead-of-time. Returns false
     * on bad address.
     */
    static bool preload(VirtAddr va);

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
     * Convenient type-safe wrapper around copyROData, where the source
     * is an untyped VirtAddr.
     */
    template <typename T>
    static inline bool copyROData(T &dest, VirtAddr src)
    {
        FlashBlockRef ref;
        return copyROData(ref, reinterpret_cast<PhysAddr>(&dest),
                          src, sizeof(T));
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
    static void setFlashSegment(const FlashRange &segment) {
        ASSERT(segment.isAligned());
        flashSeg = segment;
    }

    /**
     * Clear all user RAM. Should happen before launching a new game.
     */
    static void erase() {
        memset(userRAM, 0, RAM_SIZE_IN_BYTES);
    }

    /**
     * Reconstruct a code address, given a FlashBlock and low-level PC.
     * Used for debugging, as well as function calls.
     */
    static unsigned reconstructCodeAddr(const FlashBlockRef &ref, uint32_t pc) {
        if (ref.isHeld())
            return ref->getAddress() + (pc & FlashBlock::BLOCK_MASK)
                - flashSeg.getAddress() + VIRTUAL_FLASH_BASE;
        return 0;
    }

    /**
     * Reconstruct a RAM address, doing a Physical to Virtual translation.
     * Used for debugging only.
     */
    static VirtAddr physToVirtRAM(PhysAddr pa) {
        uintptr_t offset = pa - userRAM; 
        return (VirtAddr)offset + VIRTUAL_RAM_BASE;
    }
    static VirtAddr physToVirtRAM(Svm::reg_t pa) {
        return physToVirtRAM(reinterpret_cast<PhysAddr>(pa));
    }
    
    /**
     * Convert a flash block address to a VA in the current segment.
     * If the block address is not in the current segment at all, returns zero.
     */
    static VirtAddr flashToVirtAddr(uint32_t addr) {
        uint32_t offset = addr - flashSeg.getAddress();
        if (offset < flashSeg.getSize())
            return offset + VIRTUAL_FLASH_BASE;
        return 0;
    }

    /**
     * Convert a VA to a flash block address, using the current segment.
     * If the VA is not a valid flash address, returns zero.
     */
    static uint32_t virtToFlashAddr(VirtAddr va) {
        uint32_t offset = va - VIRTUAL_FLASH_BASE;
        if (offset < flashSeg.getSize())
            return flashSeg.getAddress() + offset;
        return 0;
    }     

    /**
     * Quick predicates to check a physical address. Used only in simulation.
     */
#ifdef SIFTEO_SIMULATOR
    static bool isAddrValid(uintptr_t pa) {
        if (FlashBlock::isAddrValid(pa))
            return true;

        uintptr_t offset = reinterpret_cast<uint8_t*>(pa) - userRAM; 
        return offset < RAM_SIZE_IN_BYTES;
    }
    static bool isAddrAligned(uintptr_t pa, int alignment) {
        return 0 == (pa & (alignment - 1));
    }
#endif

    /**
     * On the simulator, we may have a bit-ness mismatch between the
     * possibly 64-bit host and the 32-bit VM. This becomes a problem
     * with stack addresses in particular, due to how we mix and match
     * physical and virtual addresses as inputs to the SVM validate()
     * operation.
     *
     * This is largely solved by us using wider registers on 64-bit hosts,
     * but we still need a solution for when these physical stack addresses
     * are stored to memory. This function is used, on simulation only,
     * to squash 64-bit physical addresses back to 32-bit virtual ones.
     *
     * This operation only needs to be performed on 32-bit stores. Not on
     * loads, not on other kinds of stores.
     */
    static void squashPhysicalAddr(Svm::reg_t &r) {
#ifdef SIFTEO_SIMULATOR
        if (r != (uint32_t)r) {
            uintptr_t offset = reinterpret_cast<uint8_t*>(r) - userRAM; 
            if (offset < RAM_SIZE_IN_BYTES)
                r = offset + VIRTUAL_RAM_BASE;
        }
#endif
    }   

private:
    static uint8_t userRAM[RAM_SIZE_IN_BYTES] SECTION(".userram");
    static FlashRange flashSeg;
};


#endif // SVM_MEMORY_H
