/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <tr1/unordered_map>

#include "runtime.h"
#include "cube.h"
#include "neighbors.h"
#include "tasks.h"

#include <sifteo/abi.h>

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/Support/IRReader.h"
#include "llvm/Support/Debug.h"

using namespace llvm;
using namespace Sifteo;

jmp_buf Runtime::jmpExit;

bool Event::dispatchInProgress;
uint32_t Event::pending;
uint32_t Event::eventCubes[EventBits::COUNT];
bool Event::paused = false;


void Runtime::run()
{
    LLVMContext &Context = getGlobalContext();

    SMDiagnostic Err;
    Module *Mod = ParseIRFile("runtime.bc", Err, Context);
    ASSERT(Mod != NULL && "ERROR: Runtime failed to open bitcode, make sure runtime.bc is in the current directory");
        
    EngineBuilder builder(Mod);
    builder.setEngineKind(EngineKind::Interpreter);

    ExecutionEngine *EE = builder.create();
    ASSERT(EE != NULL);

    std::vector<GenericValue> args;
    Function *EntryFn = Mod->getFunction("siftmain");
    ASSERT(EntryFn != NULL);

    if (setjmp(jmpExit))
        return;

    EE->runStaticConstructorsDestructors(false);
    EE->runFunction(EntryFn, args);
    EE->runStaticConstructorsDestructors(true);    
}

void Runtime::exit()
{
    longjmp(jmpExit, 1);
}

void Event::dispatch()
{
    /*
     * Skip event dispatch if we're already in an event handler
     */

    if (dispatchInProgress || paused)
        return;
    dispatchInProgress = true;

    /*
     * Process events, by type
     */

    while (pending) {
        EventBits::ID event = (EventBits::ID)Intrinsic::CLZ(pending);

		while (eventCubes[event]) {
                _SYSCubeID slot = Intrinsic::CLZ(eventCubes[event]);
                if (event <= EventBits::LAST_CUBE_EVENT) {
                    // XXX: Not implemented in interpreter yet
                    //callCubeEvent(event, slot);
                } else if (event == EventBits::NEIGHBOR) {
                    NeighborSlot::instances[slot].computeEvents();
                }
                
                Atomic::And(eventCubes[event], ~Intrinsic::LZ(slot));
            }
        Atomic::And(pending, ~Intrinsic::LZ(event));
    }

    dispatchInProgress = false;
}

/*
 * XXX: Temporary syscall trampolines for LLVM interpreter.
 * XXX: Refactor syscalls to reduce number of different signatures?
 */

#define LLVM_SYS_VOID(name) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 0); \
         _SYS_##name(); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 1); \
         _SYS_##name((t*)GVTOP(args[0])); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_I1_PTR(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 2); \
         _SYS_##name(args[0].IntVal.getZExtValue(), (t*)GVTOP(args[1])); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_I1(name) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 1); \
         _SYS_##name(args[0].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_I2(name) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 2); \
         _SYS_##name(args[0].IntVal.getZExtValue(), args[1].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_I1(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 2); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_I2(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 3); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), args[2].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_I3(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 4); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), args[2].IntVal.getZExtValue(), args[3].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_I4(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 5); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), args[2].IntVal.getZExtValue(), \
                     args[3].IntVal.getZExtValue(), args[4].IntVal.getZExtValue()); \
         return GenericValue(); \
     }
     
#define LLVM_SYS_VOID_PTR_I_PTR(name, t, u) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 3); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), (u*)GVTOP(args[2])); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_PTR_I(name, t, u) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 3); \
         _SYS_##name((t*)GVTOP(args[0]), (u*)GVTOP(args[1]), args[2].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_I_PTR_PTR_ENUM(name, t, u, v) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 3); \
         GenericValue ret; \
         ret.IntVal = APInt(32U, (uint64_t)_SYS_##name((t*)GVTOP(args[0]), (u*)GVTOP(args[1]), \
            (v)args[2].IntVal.getZExtValue())); \
         return ret; \
     }

#define LLVM_SYS_I_I1(name) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 1); \
         GenericValue ret; \
         ret.IntVal = APInt(32U, (uint64_t)_SYS_##name(args[0].IntVal.getZExtValue())); \
         return ret; \
     }

#define LLVM_SYS_I_PTR(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 1); \
         GenericValue ret; \
         ret.IntVal = APInt(32U, (uint64_t)_SYS_##name((t*)GVTOP(args[0]))); \
         return ret; \
     }

#define LLVM_SYS_I_PTR_I1(name, t) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 2); \
         GenericValue ret; \
         ret.IntVal = APInt(32U, (uint64_t)_SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue())); \
         return ret; \
     }
     
#define LLVM_SYS_VOID_PTR_I_PTR_I1(name, t, u) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 4); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), (u*)GVTOP(args[2]), \
            args[3].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_I_PTR_I2(name, t, u) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 5); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), (u*)GVTOP(args[2]), \
            args[3].IntVal.getZExtValue(), args[4].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_PTR_I_PTR_I5(name, t, u) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 8); \
         _SYS_##name((t*)GVTOP(args[0]), args[1].IntVal.getZExtValue(), (u*)GVTOP(args[2]), \
                     args[3].IntVal.getZExtValue(), args[4].IntVal.getZExtValue(), \
                     args[5].IntVal.getZExtValue(), args[6].IntVal.getZExtValue(), \
                     args[7].IntVal.getZExtValue()); \
         return GenericValue(); \
     }

#define LLVM_SYS_VOID_F_PTR_PTR(name, t, u) \
     GenericValue lle_X__SYS_##name(FunctionType *ft, const std::vector<GenericValue> &args) \
     { \
         ASSERT(args.size() == 3); \
         _SYS_##name(args[0].FloatVal, (t*)GVTOP(args[1]), (u*)GVTOP(args[2])); \
         return GenericValue(); \
     }

extern "C" {

    LLVM_SYS_VOID(exit)
    LLVM_SYS_VOID(yield)
    LLVM_SYS_VOID(paint)
    LLVM_SYS_VOID(finish)
    LLVM_SYS_VOID_PTR(ticks_ns, int64_t)
    LLVM_SYS_VOID_PTR(vbuf_init, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR(vbuf_unlock, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR(audio_enableChannel, _SYSAudioBuffer)
    LLVM_SYS_VOID_I1(enableCubes)
    LLVM_SYS_VOID_I1(disableCubes)
    LLVM_SYS_VOID_I1(audio_stop)
    LLVM_SYS_VOID_I1(audio_pause)
    LLVM_SYS_VOID_I1(audio_resume)
    LLVM_SYS_VOID_I1_PTR(setVideoBuffer, _SYSVideoBuffer)
    LLVM_SYS_VOID_I1_PTR(loadAssets, _SYSAssetGroup)
    LLVM_SYS_VOID_I1_PTR(getAccel, _SYSAccelState)
    LLVM_SYS_VOID_I1_PTR(getNeighbors, _SYSNeighborState)
    LLVM_SYS_VOID_I1_PTR(getTilt, _SYSTiltState)
    LLVM_SYS_VOID_I1_PTR(getShake, _SYSShakeState)
    LLVM_SYS_VOID_I1_PTR(getRawNeighbors, uint8_t)
    LLVM_SYS_VOID_I1_PTR(getRawBatteryV, uint16_t)
    LLVM_SYS_VOID_I1_PTR(getCubeHWID, _SYSCubeHWID)
    LLVM_SYS_VOID_I2(solicitCubes)
    LLVM_SYS_VOID_I2(audio_setVolume)
    LLVM_SYS_VOID_PTR_I1(vbuf_lock, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I1(prng_init, _SYSPseudoRandomState)
    LLVM_SYS_VOID_PTR_I2(vbuf_poke, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I2(vbuf_pokeb, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I2(memset8, uint8_t)
    LLVM_SYS_VOID_PTR_I2(memset16, uint16_t)
    LLVM_SYS_VOID_PTR_I2(memset32, uint32_t)
    LLVM_SYS_VOID_PTR_I2(strlcat_int, char)
    LLVM_SYS_VOID_PTR_I3(vbuf_spr_resize, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I3(vbuf_spr_move, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I3(vbuf_fill, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I3(vbuf_seqi, _SYSVideoBuffer)
    LLVM_SYS_VOID_PTR_I4(strlcat_int_fixed, char)
    LLVM_SYS_VOID_PTR_I4(strlcat_int_hex, char)
    LLVM_SYS_VOID_PTR_I_PTR(vbuf_peek, _SYSVideoBuffer, uint16_t)
    LLVM_SYS_VOID_PTR_I_PTR(vbuf_peekb, _SYSVideoBuffer, uint8_t)
    LLVM_SYS_I_I1(audio_isPlaying)
    LLVM_SYS_I_I1(audio_volume)
    LLVM_SYS_I_I1(audio_pos)
    LLVM_SYS_I_PTR(prng_value, _SYSPseudoRandomState)
    LLVM_SYS_I_PTR_I1(strnlen, char)
    LLVM_SYS_I_PTR_I1(prng_valueBounded, _SYSPseudoRandomState)
    LLVM_SYS_I_PTR_PTR_ENUM(audio_play, _SYSAudioModule, _SYSAudioHandle, _SYSAudioLoopType)
    LLVM_SYS_VOID_PTR_I_PTR_I1(vbuf_write, _SYSVideoBuffer, uint16_t)
    LLVM_SYS_VOID_PTR_I_PTR_I2(vbuf_writei, _SYSVideoBuffer, uint16_t)
    LLVM_SYS_VOID_PTR_I_PTR_I5(vbuf_wrect, _SYSVideoBuffer, uint16_t)
    LLVM_SYS_VOID_PTR_PTR_I(memcpy8, uint8_t, uint8_t)
    LLVM_SYS_VOID_PTR_PTR_I(memcpy16, uint16_t, uint16_t)
    LLVM_SYS_VOID_PTR_PTR_I(memcpy32, uint32_t, uint32_t)
    LLVM_SYS_VOID_PTR_PTR_I(strlcpy, char, char)
    LLVM_SYS_VOID_PTR_PTR_I(strlcat, char, char)
    LLVM_SYS_VOID_F_PTR_PTR(sincosf, float, float)
}


/*
 * XXX: Temporary trampolines for libm functions. Need to figure out what to do with
 *      these.. some of them correspond with LLVM intrinsics, but clang seems to like
 *      generating function calls anyway.
 */
 
extern "C" {

    GenericValue lle_X_sinf(FunctionType *ft, const std::vector<GenericValue> &args)
    {
        ASSERT(args.size() == 1);
        GenericValue ret;
        ret.FloatVal = sinf(args[0].FloatVal);
        return ret;
    }

    GenericValue lle_X_cosf(FunctionType *ft, const std::vector<GenericValue> &args)
    {
        ASSERT(args.size() == 1);
        GenericValue ret;
        ret.FloatVal = cosf(args[0].FloatVal);
        return ret;
    }
}


/*
 * XXX: Temporary trampolines for other intrinsics. Clang is generating proper LLVM
 *      intrinsics for memcpy and memset, but the LLVM interpreter is turning
 *      those back into library calls.
 */
 
extern "C" {

    GenericValue lle_X_memset(FunctionType *ft, const std::vector<GenericValue> &args)
    {
        return lle_X__SYS_memset8(ft, args);
    }

    GenericValue lle_X_memcpy(FunctionType *ft, const std::vector<GenericValue> &args)
    {
        return lle_X__SYS_memcpy8(ft, args);
    }
}

/*
 * Instrumentation support, for helping us understand the execution patterns of games.
 * This uses a tiny patch to LLVM, giving us an easy and unobtrusive hook for monitoring
 * the interpreter's progress:
 *

diff --git a/lib/ExecutionEngine/Interpreter/Execution.cpp b/lib/ExecutionEngine/Interpreter/Execution.cpp
index 27917da..7341e5e 100644
--- a/lib/ExecutionEngine/Interpreter/Execution.cpp
+++ b/lib/ExecutionEngine/Interpreter/Execution.cpp
@@ -1326,6 +1326,8 @@ void Interpreter::run() {
     ++NumDynamicInsts;
 
     DEBUG(dbgs() << "About to interpret: " << I);
+    extern void xxx_InstructionHook(Instruction &I);
+    xxx_InstructionHook(I);
     visit(I);   // Dispatch to one of the visit* methods...
 #if 0
     // This is not safe, as visiting the instruction could lower it and free I.

 *
 * After applying the patch, you can select an external LLVM tree to use. For example, I
 * run the following build command:
 *
 * make LLVM_LIB=~/src/llvm/Debug+Asserts/lib LLVM_INC=~/src/llvm/include
 */

namespace llvm {
    void xxx_InstructionHook(Instruction &I)
    {
        SysTime::Ticks now = SysTime::ticks();
        
        /*
         * Right now we do only a very cursory simulation. Assume that each LLVM instruction
         * encodes to a fixed-size machine instruction, and lay them out in memory linearly in
         * the order we encounter them. Then, simulate a small flash memory cache.
         *
         * This is not even close to fully accurate. In particular, we ignore data fetches and
         * literals, and we aren't yet trying to align things to block boundaries. But it's
         * just an order-of-magnitude estimate right now.
         */
        
        const uint32_t instructionSize = 4;
        const uint32_t blockSize = 512;
        const uint32_t cacheBlocks = 16;    
        const float maxFlashKBPS = 18.0f / 8.0f * 1024.0f;

        static uint32_t iCount = 0;
        static uint32_t nextAddress = 0;
        static std::tr1::unordered_map<Instruction*, uint32_t> addrMap;
        
        iCount++;
        
        /*
         * Simulated program counter
         */

        std::tr1::unordered_map<Instruction*, uint32_t>::iterator iter = addrMap.find(&I);
        uint32_t pc;
        if (iter == addrMap.end()) {
            addrMap[&I] = pc = nextAddress;
            nextAddress += instructionSize;
        } else {
            pc = iter->second;
        }

        /*
         * Simulated block cache
         */

        static uint32_t cache[cacheBlocks];
        static SysTime::Ticks cacheTS[cacheBlocks];
        uint32_t block = 1 + pc / blockSize;
        bool hit = false;
        static uint64_t blocksLoaded;
        
        for (unsigned i = 0; i < cacheBlocks; i++) {
            if (cache[i] == block) {
                cacheTS[i] = now;
                hit = true;
                break;
            }
        }
        if (!hit) {
            // Replace the oldest block
            unsigned oldestI = 0;
            SysTime::Ticks oldestT = now;
            for (unsigned i = 0; i < cacheBlocks; i++) {
                if (cacheTS[i] < oldestT) {
                    oldestI = i;
                    oldestT = cacheTS[i];
                }
            }
            cache[oldestI] = block;
            cacheTS[oldestI] = now;
            blocksLoaded++;
            
            //dbgs() << I << "\n";
        }
        
        /*
         * Periodic stats dump
         */
        
        static SysTime::Ticks lastDumpTime; 
        SysTime::Ticks interval = now - lastDumpTime;

        if (now - lastDumpTime >= SysTime::msTicks(100)) {
            float seconds = interval / (float)SysTime::sTicks(1); 
            float blkRate = blocksLoaded / seconds;
            float kbps = blkRate * blockSize / 1024.0f;

            printf("%12.1f i/s  %10.1f blk/s  %10.2f KiB/s  %10.2f %%max  %12f %%miss  [ ",
                iCount / seconds, blkRate, kbps, kbps * 100.0f / maxFlashKBPS,
                blocksLoaded * 100.0f / iCount);
            for (unsigned i = 0; i < cacheBlocks; i++)
                printf("%02x ", cache[i]);
            printf("]\n");
            
            iCount = 0;
            blocksLoaded = 0;
            lastDumpTime = now;
        }
    }
}
