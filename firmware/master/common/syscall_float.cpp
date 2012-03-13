/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Syscalls for software floating point support.
 */

#include <math.h>
#include <sifteo/abi.h>
#include "svmmemory.h"

extern "C" {

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

}  // extern "C"
