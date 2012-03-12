/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Implementations for all syscall handlers.
 *
 * This is our front line of defense against buggy or malicious game
 * code, so here is where we need to carefully validate all input
 * values. The object implementations past this level don't
 * necessarily validate their input.
 */

#include <math.h>
#include <sifteo/machine.h>
#include <sifteo/abi.h>
#include "radio.h"
#include "cubeslots.h"
#include "cube.h"
#include "vram.h"
#include "neighbors.h"
#include "accel.h"
#include "audiomixer.h"
#include "prng.h"
#include "svmmemory.h"
#include "svmruntime.h"
#include "svmdebug.h"
#include "event.h"


extern "C" {

void _SYS_abort() {
    SvmDebug::fault(Svm::F_ABORT);
}

uint32_t _SYS_add_f32(uint32_t a, uint32_t b) {
    float r = reinterpret_cast<float&>(a) + reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_add_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) + reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_sub_f32(uint32_t a, uint32_t b) {
    float r = reinterpret_cast<float&>(a) - reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_sub_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) - reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_mul_f32(uint32_t a, uint32_t b) {
    float r = reinterpret_cast<float&>(a) * reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_mul_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) * reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_div_f32(uint32_t a, uint32_t b) {
    float r = reinterpret_cast<float&>(a) / reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_div_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) / reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint64_t _SYS_fpext_f32_f64(uint32_t a) {
    double r = reinterpret_cast<float&>(a);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_fpround_f64_f32(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    float r = reinterpret_cast<double&>(a);
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_fptosint_f32_i32(uint32_t a) {
    int32_t r = reinterpret_cast<float&>(a);
    return r;
}

uint64_t _SYS_fptosint_f32_i64(uint32_t a) {
    int64_t r = reinterpret_cast<float&>(a);
    return r;
}

uint32_t _SYS_fptosint_f64_i32(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    int32_t r = reinterpret_cast<double&>(a);
    return r;
}

uint64_t _SYS_fptosint_f64_i64(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    int64_t r = reinterpret_cast<double&>(a);
    return r;
}

uint32_t _SYS_fptouint_f32_i32(uint32_t a) {
    uint32_t r = reinterpret_cast<float&>(a);
    return r;
}

uint64_t _SYS_fptouint_f32_i64(uint32_t a) {
    uint64_t r = reinterpret_cast<float&>(a);
    return r;
}

uint32_t _SYS_fptouint_f64_i32(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint32_t r = reinterpret_cast<double&>(a);
    return r;
}

uint64_t _SYS_fptouint_f64_i64(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t r = reinterpret_cast<double&>(a);
    return r;
}

uint32_t _SYS_sinttofp_i32_f32(uint32_t a) {
    float r = reinterpret_cast<int32_t&>(a);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_sinttofp_i32_f64(uint32_t a) {
    double r = reinterpret_cast<int32_t&>(a);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_sinttofp_i64_f32(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    float r = reinterpret_cast<int64_t&>(a);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_sinttofp_i64_f64(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    double r = reinterpret_cast<int64_t&>(a);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_uinttofp_i32_f32(uint32_t a) {
    float r = a;
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_uinttofp_i32_f64(uint32_t a) {
    double r = a;
    return reinterpret_cast<uint64_t&>(r);
}
        
uint32_t _SYS_uinttofp_i64_f32(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    float r = a;
    return reinterpret_cast<uint32_t&>(r);
}
    
uint64_t _SYS_uinttofp_i64_f64(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    double r = a;
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_eq_f32(uint32_t a, uint32_t b) {
    return reinterpret_cast<float&>(a) == reinterpret_cast<float&>(b);
}

uint32_t _SYS_eq_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) == reinterpret_cast<double&>(b);
}

uint32_t _SYS_lt_f32(uint32_t a, uint32_t b) {
    return reinterpret_cast<float&>(a) < reinterpret_cast<float&>(b);
}

uint32_t _SYS_lt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) < reinterpret_cast<double&>(b);
}

uint32_t _SYS_le_f32(uint32_t a, uint32_t b) {
    return reinterpret_cast<float&>(a) <= reinterpret_cast<float&>(b);
}

uint32_t _SYS_le_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) <= reinterpret_cast<double&>(b);
}

uint32_t _SYS_ge_f32(uint32_t a, uint32_t b) {
    return reinterpret_cast<float&>(a) >= reinterpret_cast<float&>(b);
}

uint32_t _SYS_ge_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) >= reinterpret_cast<double&>(b);
}

uint32_t _SYS_gt_f32(uint32_t a, uint32_t b) {
    return reinterpret_cast<float&>(a) > reinterpret_cast<float&>(b);
}

uint32_t _SYS_gt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) > reinterpret_cast<double&>(b);
}

uint32_t _SYS_un_f32(uint32_t a, uint32_t b) {
    return isunordered(reinterpret_cast<float&>(a), reinterpret_cast<float&>(b));
}

uint32_t _SYS_un_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return isunordered(reinterpret_cast<double&>(a), reinterpret_cast<double&>(b));
}

uint32_t _SYS_fetch_and_or_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_or(p, t);
    return 0;
}

uint32_t _SYS_fetch_and_xor_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_xor(p, t);
    return 0;
}

uint32_t _SYS_fetch_and_nand_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_nand(p, t); // this issues warnings on GCC > 4.4, just ignoring for now
    return 0;
}

uint32_t _SYS_fetch_and_and_4(uint32_t *p, uint32_t t) {
    if (SvmMemory::mapRAM(p, sizeof *p))
        return __sync_fetch_and_and(p, t);
    return 0;
}

uint64_t _SYS_shl_i64(uint32_t aL, uint32_t aH, uint32_t b) {
    uint64_t a = aL | (uint64_t)aH << 32;
    return a << b;
}

uint64_t _SYS_srl_i64(uint32_t aL, uint32_t aH, uint32_t b) {
    uint64_t a = aL | (uint64_t)aH << 32;
    return a >> b;
}

int64_t _SYS_sra_i64(uint32_t aL, uint32_t aH, uint32_t b) {
    int64_t a = aL | (uint64_t)aH << 32;
    return a >> b;
}

#define MEMSET_BODY() {                                                 \
    if (SvmMemory::mapRAM(dest,                                         \
            SvmMemory::arraySize(sizeof *dest, count))) {               \
        while (count) {                                                 \
            *(dest++) = value;                                          \
            count--;                                                    \
        }                                                               \
    }                                                                   \
}

void _SYS_memset8(uint8_t *dest, uint8_t value, uint32_t count) MEMSET_BODY()
void _SYS_memset16(uint16_t *dest, uint16_t value, uint32_t count) MEMSET_BODY()
void _SYS_memset32(uint32_t *dest, uint32_t value, uint32_t count) MEMSET_BODY()

void _SYS_memcpy8(uint8_t *dest, const uint8_t *src, uint32_t count)
{
    FlashBlockRef ref;
    if (SvmMemory::mapRAM(dest, count))
        SvmMemory::copyROData(ref, dest, reinterpret_cast<SvmMemory::VirtAddr>(src), count);
}

void _SYS_memcpy16(uint16_t *dest, const uint16_t *src, uint32_t count)
{
    // Currently implemented in terms of memcpy8. We may provide a
    // separate optimized implementation of this syscall in the future.   
    _SYS_memcpy8((uint8_t*) dest, (const uint8_t*) src,
        SvmMemory::arraySize(sizeof *dest, count));
}

void _SYS_memcpy32(uint32_t *dest, const uint32_t *src, uint32_t count)
{
    // Currently implemented in terms of memcpy8. We may provide a
    // separate optimized implementation of this syscall in the future.   
    _SYS_memcpy8((uint8_t*) dest, (const uint8_t*) src,
        SvmMemory::arraySize(sizeof *dest, count));
}

int _SYS_memcmp8(const uint8_t *a, const uint8_t *b, uint32_t count)
{
    FlashBlockRef refA, refB;
    SvmMemory::VirtAddr vaA = reinterpret_cast<SvmMemory::VirtAddr>(a);
    SvmMemory::VirtAddr vaB = reinterpret_cast<SvmMemory::VirtAddr>(b);
    SvmMemory::PhysAddr paA, paB;

    while (count) {
        uint32_t chunkA = count;
        uint32_t chunkB = count;

        if (!SvmMemory::mapROData(refA, vaA, chunkA, paA))
            break;
        if (!SvmMemory::mapROData(refB, vaB, chunkB, paB))
            break;

        uint32_t chunk = MIN(chunkA, chunkB);
        vaA += chunk;
        vaB += chunk;
        count -= chunk;

        while (chunk) {
            int diff = *(paA++) - *(paB++);
            if (diff)
                return diff;
            chunk--;
        }
    }

    return 0;
}

uint32_t _SYS_strnlen(const char *str, uint32_t maxLen)
{
    FlashBlockRef ref;
    SvmMemory::VirtAddr va = reinterpret_cast<SvmMemory::VirtAddr>(str);
    SvmMemory::PhysAddr pa;
    
    uint32_t len = 0;

    while (len < maxLen) {
        uint32_t chunk = maxLen - len;
        if (!SvmMemory::mapROData(ref, va, chunk, pa))
            break;

        va += chunk;        
        while (chunk) {
            if (len == maxLen || !*pa)
                return len;
            pa++;
            len++;
            chunk--;
        }
    }
    
    return len;
}

void _SYS_strlcpy(char *dest, const char *src, uint32_t destSize)
{
    /*
     * Like the BSD strlcpy(), guaranteed to NUL-terminate.
     * We check the src pointer as we go, since the size is not known ahead of time.
     */
     
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    FlashBlockRef ref;
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    char *last = dest + destSize - 1;

    while (dest < last) {
        char c = SvmMemory::peek<uint8_t>(ref, srcVA);
        if (c) {
            *(dest++) = c;
            srcVA++;
        } else {
            break;
        }
    }

    // Guaranteed to NUL-terminate
    *dest = '\0';
}

void _SYS_strlcat(char *dest, const char *src, uint32_t destSize)
{
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    FlashBlockRef ref;
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    // Append all the bytes we can
    while (dest < last) {
        char c = SvmMemory::peek<uint8_t>(ref, srcVA);
        if (c) {
            *(dest++) = c;
            srcVA++;
        } else {
            break;
        }
    }

    // Guaranteed to NUL-termiante
    *dest = '\0';
}

void _SYS_strlcat_int(char *dest, int src, uint32_t destSize)
{
    /*
     * Integer to string conversion. Currently uses snprintf
     * (or sniprintf on embedded builds) but doesn't necessarily need to.
     */

    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    if (dest < last)
        snprintf(dest, last-dest, "%d", src);

    // Guaranteed to NUL-termiante
    *last = '\0';
}

void _SYS_strlcat_int_fixed(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize)
{
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    if (dest < last)
        snprintf(dest, last-dest, lz ? "%0*d" : "%*d", width, src);

    // Guaranteed to NUL-termiante
    *last = '\0';
}

void _SYS_strlcat_int_hex(char *dest, int src, unsigned width, unsigned lz, uint32_t destSize)
{
    if (destSize == 0 || !SvmMemory::mapRAM(dest, destSize))
        return;

    char *last = dest + destSize - 1;

    // Skip to NUL character
    while (dest < last && *dest)
        dest++;

    if (dest < last)
        snprintf(dest, last-dest, lz ? "%0*x" : "%*x", width, src);

    // Guaranteed to NUL-termiante
    *last = '\0';
}

int _SYS_strncmp(const char *a, const char *b, uint32_t count)
{
    FlashBlockRef refA, refB;
    SvmMemory::VirtAddr vaA = reinterpret_cast<SvmMemory::VirtAddr>(a);
    SvmMemory::VirtAddr vaB = reinterpret_cast<SvmMemory::VirtAddr>(b);
    SvmMemory::PhysAddr paA, paB;

    while (count) {
        uint32_t chunkA = count;
        uint32_t chunkB = count;

        if (!SvmMemory::mapROData(refA, vaA, chunkA, paA))
            break;
        if (!SvmMemory::mapROData(refB, vaB, chunkB, paB))
            break;

        uint32_t chunk = MIN(chunkA, chunkB);
        vaA += chunk;
        vaB += chunk;
        count -= chunk;

        while (chunk) {
            uint8_t aV = *(paA++);
            uint8_t bV = *(paB++);
            int diff = aV - bV;
            if (diff)
                return diff;
            if (!aV)
                return 0;
            chunk--;
        }
    }

    return 0;
}

void _SYS_sincosf(uint32_t x, float *sinOut, float *cosOut)
{
    /*
     * This syscall exists as such because it's very common, especially for
     * our game code, to compute both sine and cosine of the same angle.
     *
     * It's possible to do both operations in one step, e.g. with the
     * sincosf() function from GNU's math library.
     *
     * Right now we eschew this optimization in favor of portability,
     * but this function is in the ABI so we can optimize later without
     * breaking compatibility.
     */

    float fX = reinterpret_cast<float&>(x);

    if (SvmMemory::mapRAM(sinOut, sizeof *sinOut))
        *sinOut = ::sinf(fX);
    if (SvmMemory::mapRAM(cosOut, sizeof *cosOut))
        *cosOut = ::cosf(fX);
}

uint32_t _SYS_fmodf(uint32_t a, uint32_t b)
{
    float fA = reinterpret_cast<float&>(a);
    float fB = reinterpret_cast<float&>(b);
    float fR = (isfinite(fA) && fB != 0) ? ::fmodf(fA, fB) : NAN;
    return reinterpret_cast<uint32_t&>(fR);
}

void _SYS_prng_init(struct _SYSPseudoRandomState *state, uint32_t seed)
{
    if (SvmMemory::mapRAM(state, sizeof *state))
        PRNG::init(state, seed);
}

uint32_t _SYS_prng_value(struct _SYSPseudoRandomState *state)
{
    if (SvmMemory::mapRAM(state, sizeof *state))
        return PRNG::value(state);
    return 0;
}

uint32_t _SYS_prng_valueBounded(struct _SYSPseudoRandomState *state, uint32_t limit)
{
    if (SvmMemory::mapRAM(state, sizeof *state))
        return PRNG::valueBounded(state, limit);
    return 0;
}

void _SYS_exit(void)
{
    SvmRuntime::exit();
}

void _SYS_yield(void)
{
    Radio::halt();
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_paint(void)
{
    CubeSlots::paintCubes(CubeSlots::vecEnabled);
    SvmRuntime::dispatchEventsOnReturn();
}

void _SYS_finish(void)
{
    CubeSlots::finishCubes(CubeSlots::vecEnabled);
    SvmRuntime::dispatchEventsOnReturn();
}

int64_t _SYS_ticks_ns(void)
{
    return SysTime::ticks();
}

void _SYS_solicitCubes(_SYSCubeID min, _SYSCubeID max)
{
    CubeSlots::solicitCubes(min, max);
}

void _SYS_enableCubes(_SYSCubeIDVector cv)
{
    CubeSlots::enableCubes(CubeSlots::truncateVector(cv));
}

void _SYS_disableCubes(_SYSCubeIDVector cv)
{
    CubeSlots::disableCubes(CubeSlots::truncateVector(cv));
}

void _SYS_setVideoBuffer(_SYSCubeID cid, struct _SYSVideoBuffer *vbuf)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf) && CubeSlots::validID(cid))
        CubeSlots::instances[cid].setVideoBuffer(vbuf);
}

void _SYS_loadAssets(_SYSCubeID cid, _SYSAssetGroup *group)
{
    if (SvmMemory::mapRAM(group, sizeof *group) && CubeSlots::validID(cid))
        CubeSlots::instances[cid].loadAssets(group);
}

struct _SYSAccelState _SYS_getAccel(_SYSCubeID cid)
{
    struct _SYSAccelState r = { 0 };
    if (CubeSlots::validID(cid))
        CubeSlots::instances[cid].getAccelState(&r);
    return r;
}

void _SYS_getNeighbors(_SYSCubeID cid, struct _SYSNeighborState *state) {
    if (SvmMemory::mapRAM(state, sizeof *state) && CubeSlots::validID(cid)) {
        NeighborSlot::instances[cid].getNeighborState(state);
    }
}

struct _SYSTiltState _SYS_getTilt(_SYSCubeID cid)
{
    struct _SYSTiltState r = { 0 };
    if (CubeSlots::validID(cid))
        AccelState::instances[cid].getTiltState(&r);
    return r;
}

_SYSShakeState _SYS_getShake(_SYSCubeID cid)
{
    _SYSShakeState r = NOT_SHAKING;
    if (CubeSlots::validID(cid))
        AccelState::instances[cid].getShakeState(&r);
    return r;
}

void _SYS_getRawNeighbors(_SYSCubeID cid, uint8_t buf[4])
{
    // XXX: Temporary for testing/demoing
    if (SvmMemory::mapRAM(buf, sizeof buf) && CubeSlots::validID(cid))
        memcpy(buf, CubeSlots::instances[cid].getRawNeighbors(), 4);
}

uint8_t _SYS_isTouching(_SYSCubeID cid)
{
    if (CubeSlots::validID(cid)) {
        return CubeSlots::instances[cid].isTouching();
    }
    return 0;
}

uint16_t _SYS_getRawBatteryV(_SYSCubeID cid)
{
    // XXX: Temporary for testing. Master firmware should give cooked battery percentage.
    if (CubeSlots::validID(cid))
        return CubeSlots::instances[cid].getRawBatteryV();
    return 0;
}

void _SYS_getCubeHWID(_SYSCubeID cid, _SYSCubeHWID *hwid)
{
    // XXX: Maybe temporary?

    // XXX: Right now this is only guaranteed to be known after asset downloading, since
    //      there is no code yet to explicitly request it (via a flash reset)

    if (SvmMemory::mapRAM(hwid, sizeof hwid) && CubeSlots::validID(cid))
        *hwid = CubeSlots::instances[cid].getHWID();
}

void _SYS_vbuf_init(_SYSVideoBuffer *vbuf)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::init(*vbuf);
    }
}

void _SYS_vbuf_lock(_SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        VRAM::lock(*vbuf, addr);
    }
}

void _SYS_vbuf_unlock(_SYSVideoBuffer *vbuf)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::unlock(*vbuf);
    }
}

void _SYS_vbuf_poke(_SYSVideoBuffer *vbuf, uint16_t addr, uint16_t word)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        VRAM::poke(*vbuf, addr, word);
    }
}

void _SYS_vbuf_pokeb(_SYSVideoBuffer *vbuf, uint16_t addr, uint8_t byte)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateByteAddr(addr);
        VRAM::pokeb(*vbuf, addr, byte);
    }
}

uint16_t _SYS_vbuf_peek(const _SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateWordAddr(addr);
        return VRAM::peek(*vbuf, addr);
    }
    return 0;
}

uint8_t _SYS_vbuf_peekb(const _SYSVideoBuffer *vbuf, uint16_t addr)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        VRAM::truncateByteAddr(addr);
        return VRAM::peekb(*vbuf, addr);
    }
    return 0;
}

void _SYS_vbuf_fill(struct _SYSVideoBuffer *vbuf, uint16_t addr,
                    uint16_t word, uint16_t count)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        while (count) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, word);
            count--;
            addr++;
        }
    }
}

void _SYS_vbuf_write(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t count)
{
    if (!SvmMemory::mapRAM(vbuf, sizeof *vbuf))
        return;

    FlashBlockRef ref;
    uint32_t bytes = SvmMemory::arraySize(sizeof *src, count);
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    SvmMemory::PhysAddr srcPA;

    while (bytes) {
        uint32_t chunk = bytes;
        if (!SvmMemory::mapROData(ref, srcVA, chunk, srcPA))
            return;

        srcVA += chunk;
        bytes -= chunk;

        while (chunk) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, *reinterpret_cast<uint16_t*>(srcPA));
            addr++;

            chunk -= sizeof(uint16_t);
            srcPA += sizeof(uint16_t);
        }
    }
}

void _SYS_vbuf_writei(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src,
                      uint16_t offset, uint16_t count)
{
    if (!SvmMemory::mapRAM(vbuf, sizeof *vbuf))
        return;

    FlashBlockRef ref;
    uint32_t bytes = SvmMemory::arraySize(sizeof *src, count);
    SvmMemory::VirtAddr srcVA = reinterpret_cast<SvmMemory::VirtAddr>(src);
    SvmMemory::PhysAddr srcPA;

    ASSERT((bytes & 1) == 0);
    while (bytes) {
        uint32_t chunk = bytes;
        if (!SvmMemory::mapROData(ref, srcVA, chunk, srcPA))
            return;

        ASSERT((chunk & 1) == 0);
        srcVA += chunk;
        bytes -= chunk;

        while (chunk) {
            uint16_t index = offset + *reinterpret_cast<uint16_t*>(srcPA);

            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, VRAM::index14(index));
            addr++;

            chunk -= sizeof(uint16_t);
            srcPA += sizeof(uint16_t);
        }
    }
}

void _SYS_vbuf_seqi(struct _SYSVideoBuffer *vbuf, uint16_t addr, uint16_t index, uint16_t count)
{
    if (SvmMemory::mapRAM(vbuf, sizeof *vbuf)) {
        while (count) {
            VRAM::truncateWordAddr(addr);
            VRAM::poke(*vbuf, addr, VRAM::index14(index));
            count--;
            addr++;
            index++;
        }
    }
}

void _SYS_vbuf_wrect(struct _SYSVideoBuffer *vbuf, uint16_t addr, const uint16_t *src, uint16_t offset, uint16_t count,
                     uint16_t lines, uint16_t src_stride, uint16_t addr_stride)
{
    /*
     * Shortcut for a rectangular blit, comprised of multiple calls to SYS_vbuf_writei.
     * We save the pointer validation for writei, since we want to do it per-scanline anyway.
     */

    while (lines--) {
        _SYS_vbuf_writei(vbuf, addr, src, offset, count);
        src += src_stride;
        addr += addr_stride;
    }
}

void _SYS_vbuf_spr_resize(struct _SYSVideoBuffer *vbuf, unsigned id, unsigned width, unsigned height)
{
    // Address validation occurs after these calculations, in _SYS_vbuf_poke.
    // Sprite ID validation is implicit.

    uint8_t xb = -(int)width;
    uint8_t yb = -(int)height;
    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                     sizeof(_SYSSpriteInfo)/2 * id );

    _SYS_vbuf_poke(vbuf, addr, word);
}

void _SYS_vbuf_spr_move(struct _SYSVideoBuffer *vbuf, unsigned id, int x, int y)
{
    // Address validation occurs after these calculations, in _SYS_vbuf_poke.
    // Sprite ID validation is implicit.

    uint8_t xb = -x;
    uint8_t yb = -y;
    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id );

    _SYS_vbuf_poke(vbuf, addr, word);
}

void _SYS_audio_enableChannel(struct _SYSAudioBuffer *buffer)
{
    if (SvmMemory::mapRAM(buffer, sizeof(*buffer)))
        AudioMixer::instance.enableChannel(buffer);
}

uint8_t _SYS_audio_play(const struct _SYSAudioModule *mod, _SYSAudioHandle *h, enum _SYSAudioLoopType loop)
{
    _SYSAudioModule modCopy;
    if (SvmMemory::copyROData(modCopy, mod) && SvmMemory::mapRAM(h, sizeof(*h))) {
        return AudioMixer::instance.play(&modCopy, h, loop);
    }
    return false;
}

uint8_t _SYS_audio_isPlaying(_SYSAudioHandle h)
{
    return AudioMixer::instance.isPlaying(h);
}

void _SYS_audio_stop(_SYSAudioHandle h)
{
    AudioMixer::instance.stop(h);
}

void _SYS_audio_pause(_SYSAudioHandle h)
{
    AudioMixer::instance.pause(h);
}

void _SYS_audio_resume(_SYSAudioHandle h)
{
    AudioMixer::instance.resume(h);
}

int _SYS_audio_volume(_SYSAudioHandle h)
{
    return AudioMixer::instance.volume(h);
}

void _SYS_audio_setVolume(_SYSAudioHandle h, int volume)
{
    AudioMixer::instance.setVolume(h, volume);
}

uint32_t _SYS_audio_pos(_SYSAudioHandle h)
{
    return AudioMixer::instance.pos(h);
}

void _SYS_setVector(_SYSVectorID vid, void *handler, void *context)
{
    if (vid < _SYS_NUM_VECTORS)
        Event::setVector(vid, handler, context);
}

void *_SYS_getVectorHandler(_SYSVectorID vid)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::getVectorHandler(vid);
    return NULL;
}

void *_SYS_getVectorContext(_SYSVectorID vid)
{
    if (vid < _SYS_NUM_VECTORS)
        return Event::getVectorContext(vid);
    return NULL;
}

void _SYS_log(uint32_t t, uintptr_t v1, uintptr_t v2, uintptr_t v3,
    uintptr_t v4, uintptr_t v5, uintptr_t v6, uintptr_t v7)
{
    SvmLogTag tag(t);
    uint32_t type = tag.getType();
    uint32_t arity = tag.getArity();

    SvmMemory::squashPhysicalAddr(v1);
    SvmMemory::squashPhysicalAddr(v2);
    SvmMemory::squashPhysicalAddr(v3);
    SvmMemory::squashPhysicalAddr(v4);
    SvmMemory::squashPhysicalAddr(v5);
    SvmMemory::squashPhysicalAddr(v6);
    SvmMemory::squashPhysicalAddr(v7);

    switch (type) {

        // Stow all arguments, plus the log tag. The post-processor
        // will do some printf()-like formatting on the stored arguments.
        case _SYS_LOGTYPE_FMT: {
            uint32_t *buffer = SvmDebug::logReserve(tag);        
            switch (arity) {
                case 7: buffer[6] = v7;
                case 6: buffer[5] = v6;
                case 5: buffer[4] = v5;
                case 4: buffer[3] = v4;
                case 3: buffer[2] = v3;
                case 2: buffer[1] = v2;
                case 1: buffer[0] = v1;
                case 0:
                default: break;
            }
            SvmDebug::logCommit(tag, buffer, arity * sizeof(uint32_t));
            return;
        }

        // String messages: Only v1 is used (as a pointer), and we emit
        // zero or more log records until we hit the NUL terminator.
        case _SYS_LOGTYPE_STRING: {
            const unsigned chunkSize = SvmDebug::LOG_BUFFER_BYTES;
            FlashBlockRef ref;
            for (;;) {
                uint32_t *buffer = SvmDebug::logReserve(tag);
                if (!SvmMemory::copyROData(ref, (SvmMemory::PhysAddr)buffer,
                                    (SvmMemory::VirtAddr)v1, chunkSize)) {
                    SvmDebug::fault(F_LOG_FETCH);
                    return;
                }

                // strnlen is not easily available on mingw...
                const char *str = reinterpret_cast<const char*>(buffer);
                const char *end = static_cast<const char*>(memchr(str, '\0', chunkSize));
                uint32_t bytes = end ? (size_t) (end - str) : chunkSize;

                SvmDebug::logCommit(SvmLogTag(tag, bytes), buffer, bytes);
                if (bytes < chunkSize)
                    return;
            }
        }
        
        // Hex dumps: Like strings, v1 is used as a pointer. The inline
        // parameter from 'tag' is the length of the dump, in bytes. If we're
        // dumping more than what fits in a single log record, emit multiple
        // log records.
        case _SYS_LOGTYPE_HEXDUMP: {
            FlashBlockRef ref;
            uint32_t remaining = tag.getParam();
            while (remaining) {
                uint32_t chunkSize = MIN(SvmDebug::LOG_BUFFER_BYTES, remaining);
                uint32_t *buffer = SvmDebug::logReserve(tag);
                if (!SvmMemory::copyROData(ref, (SvmMemory::PhysAddr)buffer,
                                    (SvmMemory::VirtAddr)v1, chunkSize)) {
                    SvmDebug::fault(F_LOG_FETCH);
                    return;
                }
                SvmDebug::logCommit(SvmLogTag(tag, chunkSize), buffer, chunkSize);
                remaining -= chunkSize;
            }
            return;
        }

        default:
            ASSERT(0 && "Unknown _SYS_log() tag type");
            return;
    }
}


}  // extern "C"
