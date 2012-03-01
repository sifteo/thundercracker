/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_RUNTIME_H
#define _SIFTEO_RUNTIME_H

#include <setjmp.h>
#include <sifteo/abi.h>
#include <sifteo/machine.h>

/**
 * Game code runtime environment
 */

class Runtime {
 public:
    static void run();
    static void exit();

    /*
        Ensure that a read-only pointer is valid, and translate if necessary.
        Read-write pointers may reference any valid flash or ram memory.
    */
    template <typename T>
    static const bool validateReadOnly(T &ptr, uint32_t size, bool allowNULL = false)
    {
        if (!allowNULL && !ptr)
            return false;

        ptr = ptr;
        return true;
    }

    /*
        Ensure that a read-write pointer is valid, and translate if necessary.
        Read-write pointers cannot reference any flash memory.
    */
    template <typename T>
    static const bool validateReadWrite(T &ptr, uint32_t size, bool allowNULL = false)
    {
        if (!allowNULL && !ptr)
            return false;

        ptr = ptr;
        return true;
    }

    static bool checkUserPointer(const void *ptr, uint32_t size, bool allowNULL=false)
    {
        /*
         * XXX: Validate a memory address that was provided to us by the game,
         *      make sure it isn't outside the game's sandbox region. This code
         *      MUST use overflow-safe arithmetic!
         *
         * XXX: We should probably split this into two variants for
         *      two different kinds of user pointers: RAM pointers
         *      (read/write) and virtual flash addresses (read-only).
         *      The variant for virtual flash addresses will also need
         *      to map those addresses (via our cache) into local SRAM
         *      temporarily. Yes, it's yet another software virtual
         *      memory subsystem. VMware deja vu.
         *
         * Also checks for NULL pointers, assuming allowNULL isn't true.
         *
         * May be called anywhere, at any time, including from interrupt context.
         */

        if (!ptr && !allowNULL)
            return false;

        return true;
    }
    
    static bool checkUserArrayPointer(const void *ptr, uint32_t itemSize, uint32_t count, bool allowNULL=false)
    {
        /*
         * Check a pointer to a variable-sized array. Also checks for integer
         * overflow when calculating the total size of the array.
         *
         * The current implementation assumes object sizes will fit in 16 bits.
         * This test is sufficient to ensure that the multiplication will
         * never overflow, without resorting to hardware-specific overflow detection
         * checks, division, or 64-bit multiplication.
         */

        if (itemSize >= 0x10000)
            return false;
        
        if (count >= 0x10000)
            return false;
        
        return checkUserPointer(ptr, itemSize * count, allowNULL);
    }

 private:
    static jmp_buf jmpExit;
};


/**
 * Event dispatcher
 *
 * Pending events are tracked with a hierarchy of change bitmaps.
 * Bitmaps should always be set from most specific to least specific.
 * For example, with asset downloading, an individual cube's bit
 * in assetDoneCubes
 */

class Event {
 public:
    static void dispatch();
    
    static void setPending(_SYSVectorID vid, _SYSCubeID cid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
        Sifteo::Atomic::SetLZ(pending, vid);
        Sifteo::Atomic::SetLZ(vectors[vid].cubesPending, cid);
    }
    
    static void setVector(_SYSVectorID vid, void *handler, void *context) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        vectors[vid].handler = handler;
        vectors[vid].context = context;
    }
    
    static void *getVectorHandler(_SYSVectorID vid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        return vectors[vid].handler;
    }

    static void *getVectorContext(_SYSVectorID vid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        return vectors[vid].context;
    }
	
    static inline void callCubeEvent(_SYSVectorID vid, _SYSCubeID cid) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        ASSERT(cid < _SYS_NUM_CUBE_SLOTS);
        ASSERT(Sifteo::Intrinsic::LZ(vid) & _SYS_CUBE_EVENTS);
#if 0 // XXX: Runtime support for events
        VectorInfo &vi = vectors[vid];
        _SYSCubeEvent fn = (_SYSCubeEvent) vi.handler;
        if (fn)
            fn(vi.context, cid);
#endif
    }

    static inline void callNeighborEvent(_SYSVectorID vid,
        _SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1) {
        ASSERT(vid < _SYS_NUM_VECTORS);
        ASSERT(Sifteo::Intrinsic::LZ(vid) & _SYS_NEIGHBOR_EVENTS);
#if 0 // XXX: Runtime support for events
        VectorInfo &vi = vectors[vid];
        _SYSNeighborEvent fn = (_SYSNeighborEvent) vi.handler;
        if (fn)
            fn(vi.context, c0, s0, c1, s1);
#endif
    }
	
    static uint32_t pending;            /// CLZ map of all pending events
    static bool dispatchInProgress;     /// Reentrancy detector

 private:
     
    struct VectorInfo {
        void *handler;
        void *context;
        union {                                 /// Type-specific state:
            _SYSCubeIDVector cubesPending;      /// CLZ map of pending cubes
        };
    };

    static VectorInfo vectors[_SYS_NUM_VECTORS];
};


#endif
