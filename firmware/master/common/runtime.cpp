/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "runtime.h"
#include "cube.h"
#include "neighbors.h"
#ifndef SIFTEO_SIMULATOR
    #include "tasks.h"
#endif

#include "llvm/LLVMContext.h"
#include "llvm/Module.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/Support/IRReader.h"

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
                    callCubeEvent(event, slot);
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

#if 0
    uint8_t _SYS_audio_isPlaying(_SYSAudioHandle h);
    int  _SYS_audio_volume(_SYSAudioHandle h);

     void _SYS_solicitCubes(_SYSCubeID min, _SYSCubeID max);
    


     // XXX: Temporary for testing/demoing
     void _SYS_getRawNeighbors(_SYSCubeID cid, uint8_t buf[4]);
     void _SYS_getRawBatteryV(_SYSCubeID cid, uint16_t *v);
     void _SYS_getCubeHWID(_SYSCubeID cid, struct _SYSCubeHWID *hwid);

     void _SYS_vbuf_lock(struct _SYSVideoBuffer *vbuf, uint16_t addr);
     void _SYS_vbuf_poke(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word);
     void _SYS_vbuf_pokeb(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte);
     void _SYS_vbuf_peek(const struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t *word);
     void _SYS_vbuf_peekb(const struct _SYSVideoBuffer *vbuf, uint16_t addr, uint8_t *byte);
     void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word, uint16_t count);
     void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count);
     void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count);
     void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count);
     void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count,
                          uint16_t lines, uint16_t src_stride, uint16_t addr_stride);
     void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height);
     void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y);

     //uint8_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop);
     uint8_t _SYS_audio_play(struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop);
     void _SYS_audio_setVolume(_SYSAudioHandle h, int volume);
     uint32_t _SYS_audio_pos(_SYSAudioHandle h);
 #endif
}
 
/*
 * XXX: Temporary trampolines for libc functions that have leaked into games.
 *      These should be redesigned in terms of syscalls and our ABI!
 */
 
extern "C" {

    GenericValue lle_X_memset(FunctionType *ft, const std::vector<GenericValue> &args)
    {
        ASSERT(args.size() == 3);
        memset(GVTOP(args[0]), args[1].IntVal.getZExtValue(), args[2].IntVal.getZExtValue());
        return GenericValue();
    }

    GenericValue lle_X_rand_r(FunctionType *ft, const std::vector<GenericValue> &args)
    {
        ASSERT(args.size() == 1);
        GenericValue ret;
        ret.IntVal = rand_r((unsigned *)GVTOP(args[0]));
        return ret;
    }
}