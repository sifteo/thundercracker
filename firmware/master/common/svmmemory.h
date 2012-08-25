/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_MEMORY_H
#define SVM_MEMORY_H

#include "macros.h"
#include "svm.h"
#include "flash_blockcache.h"
#include "flash_map.h"

#include <string.h>


class SvmMemory {
public:
    // Flash
    static const unsigned SEGMENT_0_VA = 0x80000000;
    static const unsigned SEGMENT_1_VA = 0xc0000000;
    static const unsigned NUM_FLASH_SEGMENTS = 2;

    // RAM
    static const unsigned RAM_SIZE_IN_BYTES = 32 * 1024;
    static const unsigned VIRTUAL_RAM_BASE = 0x10000;
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
     * Assumes 'va' is a flash address in segment 0. High bits are ignored.
     */
    static bool mapROCode(FlashBlockRef &ref, VirtAddr va, PhysAddr &pa);

    /**
     * Copy out read-only data into arbitrary physical RAM. This is a more
     * convenient alternative for small data structures that are not necessarily
     * block aligned.
     */
    static bool copyROData(FlashBlockRef &ref, PhysAddr dest, VirtAddr src, uint32_t length);

    /**
     * Copy out read-only data into arbitrary physical RAM, stopping when we hit
     * a NUL terminator. Guaranteed to NUL-terminate the destination string
     * unless a fault occurs.
     */
    static bool strlcpyROData(FlashBlockRef &ref, char *dest, VirtAddr src, uint32_t destSize);

    /**
     * Perform a 32-bit CRC over read-only data in virtual memory.
     * The address does not need to be aligned at all, and the length
     * can be any arbitrary number of bytes.
     *
     * The CRC is padded with 0xFF bytes until its total length
     * is a multiple of 'alignment', which must be >= 4 and a power of two.
     *
     * Returns true on success.
     */
    static bool crcROData(FlashBlockRef &ref, VirtAddr src, uint32_t length,
        uint32_t &crc, unsigned alignment = 4);

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
    static ALWAYS_INLINE bool copyROData(T &dest, const T *src)
    {
        FlashBlockRef ref;
        return copyROData(ref, reinterpret_cast<PhysAddr>(&dest),
                          reinterpret_cast<VirtAddr>(src), sizeof(T));
    }

    /**
     * Convenient type-safe wrapper around copyROData,
     * with a caller-supplied FlashBlockRef.
     */
    template <typename T>
    static ALWAYS_INLINE bool copyROData(FlashBlockRef &ref, T &dest, const T *src)
    {
        return copyROData(ref, reinterpret_cast<PhysAddr>(&dest),
                          reinterpret_cast<VirtAddr>(src), sizeof(T));
    }

    /**
     * Convenient type-safe wrapper around copyROData, where the source
     * is an untyped VirtAddr.
     */
    template <typename T>
    static ALWAYS_INLINE bool copyROData(T &dest, VirtAddr src)
    {
        FlashBlockRef ref;
        return copyROData(ref, reinterpret_cast<PhysAddr>(&dest),
                          src, sizeof(T));
    }

    /**
     * Convenient type-safe wrapper around copyROData, where the source
     * is an untyped VirtAddr and with a caller-supplied FlashBlockRef.
     */
    template <typename T>
    static ALWAYS_INLINE bool copyROData(FlashBlockRef &ref, T &dest, VirtAddr src)
    {
        return copyROData(ref, reinterpret_cast<PhysAddr>(&dest),
                          src, sizeof(T));
    }

    /**
     * Convenience functions to read a single value from RAM or Flash.
     * Returns NULL on failure.
     */
    template <typename T>
    static inline T* peek(FlashBlockRef &ref, VirtAddr va)
    {
        uint32_t length = sizeof(T);
        PhysAddr pa;
        
        if (mapROData(ref, va, length, pa) && length == sizeof(T))
            return reinterpret_cast<T*>(pa);
        else
            return NULL;
    }

    /**
     * Set the bounds of the flash memory region reachable through SVM.
     * This is initialized to the current binary's RODATA segment by our ELF
     * loader.
     */
    static ALWAYS_INLINE void setFlashSegment(unsigned index, const FlashMapSpan &span) {
        ASSERT(index < NUM_FLASH_SEGMENTS);
        flashSeg[index] = span;
    }

    /**
     * Clear all user RAM and unmap all Flash segments.
     * Should happen before launching a new game.
     */
    static void erase() {
        memset(userRAM, 0, RAM_SIZE_IN_BYTES);
        for (unsigned i = 0; i < arraysize(flashSeg); i++)
            setFlashSegment(i, FlashMapSpan::empty());
    }

    /**
     * Reconstruct a code address, given a FlashBlock and low-level PC.
     * Used for debugging, as well as function calls. Assumes 'pc' is in
     * the default flash segment.
     */
    static unsigned reconstructCodeAddr(const FlashBlockRef &ref, uint32_t pc);

    /**
     * Reconstruct a RAM address, doing a Physical to Virtual translation.
     * Used for debugging only.
     */
    static ALWAYS_INLINE VirtAddr physToVirtRAM(PhysAddr pa) {
        uintptr_t offset = pa - userRAM; 
        return (VirtAddr)offset + VIRTUAL_RAM_BASE;
    }
    static ALWAYS_INLINE VirtAddr physToVirtRAM(Svm::reg_t pa) {
        return physToVirtRAM(reinterpret_cast<PhysAddr>(pa));
    }
    
    /**
     * Convert a flash block address to a VA in any valid segment.
     * If the block address is not in any segment at all, returns zero.
     */
    static VirtAddr flashToVirtAddr(uint32_t addr) {
        STATIC_ASSERT(arraysize(flashSeg) == 2);
        FlashMapSpan::ByteOffset offset;
        if (flashSeg[0].flashAddrToOffset(addr, offset))
            return offset + SEGMENT_0_VA;
        if (flashSeg[1].flashAddrToOffset(addr, offset))
            return offset + SEGMENT_1_VA;
        return 0;
    }

    /**
     * Convert a VA to a flash block address in any valid segment.
     * If the VA is not a valid flash address, returns zero.
     */
    static uint32_t virtToFlashAddr(VirtAddr va) {
        STATIC_ASSERT(arraysize(flashSeg) == 2);
        FlashMapSpan::FlashAddr addr;
        if (flashSeg[0].offsetToFlashAddr(va - SEGMENT_0_VA, addr))
            return addr;
        if (flashSeg[1].offsetToFlashAddr(va - SEGMENT_1_VA, addr))
            return addr;
        return 0;
    }     

    /**
     * Convert the VA to a segment number, or an invalid segment number
     * if the VA is not a valid flash address.
     */
    static unsigned virtToFlashSegment(VirtAddr va) {
        STATIC_ASSERT(arraysize(flashSeg) == 2);
        if (flashSeg[0].offsetIsValid(va - SEGMENT_0_VA))
            return 0;
        if (flashSeg[1].offsetIsValid(va - SEGMENT_1_VA))
            return 1;
        return unsigned(-1);
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
    static ALWAYS_INLINE uint32_t squashPhysicalAddr(Svm::reg_t r) {
#ifdef SIFTEO_SIMULATOR
        if (r != (uint32_t)r) {
            uintptr_t offset = reinterpret_cast<uint8_t*>(r) - userRAM; 
            if (offset < RAM_SIZE_IN_BYTES)
                r = offset + VIRTUAL_RAM_BASE;
        }
#endif
        return r;
    }

private:
    static uint8_t userRAM[RAM_SIZE_IN_BYTES] SECTION(".userram");
    static FlashMapSpan flashSeg[NUM_FLASH_SEGMENTS];
};


#endif // SVM_MEMORY_H
