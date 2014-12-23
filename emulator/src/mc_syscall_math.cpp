/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2012 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/*
 * Emulator-specific syscalls for math library support.
 *
 * This file emulates floating point and integer operations which,
 * on hardware, are mostly forwarded directly to the software math library.
 */

#include <math.h>
#include <sifteo/abi.h>
#include "svmmemory.h"

extern "C" {

uint32_t _SYS_add_f32(uint32_t a, uint32_t b)
{
    float r = reinterpret_cast<float&>(a) + reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_add_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) + reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_sub_f32(uint32_t a, uint32_t b)
{
    float r = reinterpret_cast<float&>(a) - reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_sub_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) - reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_mul_f32(uint32_t a, uint32_t b)
{
    float r = reinterpret_cast<float&>(a) * reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_mul_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) * reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_div_f32(uint32_t a, uint32_t b)
{
    float r = reinterpret_cast<float&>(a) / reinterpret_cast<float&>(b);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_div_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double r = reinterpret_cast<double&>(a) / reinterpret_cast<double&>(b);
    return reinterpret_cast<uint64_t&>(r);
}

uint64_t _SYS_fpext_f32_f64(uint32_t a)
{
    double r = reinterpret_cast<float&>(a);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_fpround_f64_f32(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    float r = reinterpret_cast<double&>(a);
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_fptosint_f32_i32(uint32_t a)
{
    int32_t r = reinterpret_cast<float&>(a);
    return r;
}

uint64_t _SYS_fptosint_f32_i64(uint32_t a)
{
    int64_t r = reinterpret_cast<float&>(a);
    return r;
}

uint32_t _SYS_fptosint_f64_i32(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    int32_t r = reinterpret_cast<double&>(a);
    return r;
}

uint64_t _SYS_fptosint_f64_i64(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    int64_t r = reinterpret_cast<double&>(a);
    return r;
}

uint32_t _SYS_fptouint_f32_i32(uint32_t a)
{
    uint32_t r = reinterpret_cast<float&>(a);
    return r;
}

uint64_t _SYS_fptouint_f32_i64(uint32_t a)
{
    uint64_t r = reinterpret_cast<float&>(a);
    return r;
}

uint32_t _SYS_fptouint_f64_i32(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint32_t r = reinterpret_cast<double&>(a);
    return r;
}

uint64_t _SYS_fptouint_f64_i64(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t r = reinterpret_cast<double&>(a);
    return r;
}

uint32_t _SYS_sinttofp_i32_f32(uint32_t a)
{
    float r = reinterpret_cast<int32_t&>(a);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_sinttofp_i32_f64(uint32_t a)
{
    double r = reinterpret_cast<int32_t&>(a);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_sinttofp_i64_f32(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    float r = reinterpret_cast<int64_t&>(a);
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_sinttofp_i64_f64(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    double r = reinterpret_cast<int64_t&>(a);
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_uinttofp_i32_f32(uint32_t a)
{
    float r = a;
    return reinterpret_cast<uint32_t&>(r);
}

uint64_t _SYS_uinttofp_i32_f64(uint32_t a)
{
    double r = a;
    return reinterpret_cast<uint64_t&>(r);
}
        
uint32_t _SYS_uinttofp_i64_f32(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    float r = a;
    return reinterpret_cast<uint32_t&>(r);
}
    
uint64_t _SYS_uinttofp_i64_f64(uint32_t aL, uint32_t aH) {
    uint64_t a = aL | (uint64_t)aH << 32;
    double r = a;
    return reinterpret_cast<uint64_t&>(r);
}

uint32_t _SYS_eq_f32(uint32_t a, uint32_t b)
{
    return reinterpret_cast<float&>(a) == reinterpret_cast<float&>(b);
}

uint32_t _SYS_eq_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) == reinterpret_cast<double&>(b);
}

uint32_t _SYS_lt_f32(uint32_t a, uint32_t b)
{
    return reinterpret_cast<float&>(a) < reinterpret_cast<float&>(b);
}

uint32_t _SYS_lt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) < reinterpret_cast<double&>(b);
}

uint32_t _SYS_le_f32(uint32_t a, uint32_t b)
{
    return reinterpret_cast<float&>(a) <= reinterpret_cast<float&>(b);
}

uint32_t _SYS_le_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) <= reinterpret_cast<double&>(b);
}

uint32_t _SYS_ge_f32(uint32_t a, uint32_t b)
{
    return reinterpret_cast<float&>(a) >= reinterpret_cast<float&>(b);
}

uint32_t _SYS_ge_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) >= reinterpret_cast<double&>(b);
}

uint32_t _SYS_gt_f32(uint32_t a, uint32_t b)
{
    return reinterpret_cast<float&>(a) > reinterpret_cast<float&>(b);
}

uint32_t _SYS_gt_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return reinterpret_cast<double&>(a) > reinterpret_cast<double&>(b);
}

uint32_t _SYS_un_f32(uint32_t a, uint32_t b)
{
    return isunordered(reinterpret_cast<float&>(a), reinterpret_cast<float&>(b));
}

uint32_t _SYS_un_f64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return isunordered(reinterpret_cast<double&>(a), reinterpret_cast<double&>(b));
}

uint32_t _SYS_fmodf(uint32_t a, uint32_t b)
{
    float fA = reinterpret_cast<float&>(a);
    float fB = reinterpret_cast<float&>(b);
    float fR = (isfinite(fA) && fB != 0) ? ::fmodf(fA, fB) : NAN;
    return reinterpret_cast<uint32_t&>(fR);
}

uint32_t _SYS_powf(uint32_t a, uint32_t b)
{
    float fA = reinterpret_cast<float&>(a);
    float fB = reinterpret_cast<float&>(b);
    float fR = powf(fA, fB);
    return reinterpret_cast<uint32_t&>(fR);
}

uint32_t _SYS_sqrtf(uint32_t a)
{
    float r = sqrtf(reinterpret_cast<float&>(a));
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_logf(uint32_t a)
{
    float r = logf(reinterpret_cast<float&>(a));
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_sinf(uint32_t a)
{
    float r = sinf(reinterpret_cast<float&>(a));
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_cosf(uint32_t a)
{
    float r = cosf(reinterpret_cast<float&>(a));
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_tanf(uint32_t a)
{
    float r = tanf(reinterpret_cast<float&>(a));
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_atanf(uint32_t a)
{
    float r = atanf(reinterpret_cast<float&>(a));
    return reinterpret_cast<uint32_t&>(r);
}

uint32_t _SYS_atan2f(uint32_t y, uint32_t x)
{
    float fY = reinterpret_cast<float&>(y);
    float fX = reinterpret_cast<float&>(x);
    float fR = atan2f(fY, fX);
    return reinterpret_cast<uint32_t&>(fR);
}

uint64_t _SYS_fmod(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double fA = reinterpret_cast<double&>(a);
    double fB = reinterpret_cast<double&>(b);
    double r = (isfinite(fA) && fB != 0) ? ::fmod(fA, fB) : NAN;
    return reinterpret_cast<uint64_t&>(r);
}

uint64_t _SYS_pow(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    double fA = reinterpret_cast<double&>(a);
    double fB = reinterpret_cast<double&>(b);
    double r = pow(fA, fB);
    return reinterpret_cast<uint64_t&>(r);
}

uint64_t _SYS_sqrt(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    double r = sqrt(reinterpret_cast<double&>(a));
    return reinterpret_cast<uint64_t&>(r);
}

uint64_t _SYS_logd(uint32_t aL, uint32_t aH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    double r = log(reinterpret_cast<double&>(a));
    return reinterpret_cast<uint64_t&>(r);
}

void _SYS_sincosf(uint32_t x, float *sinOut, float *cosOut)
{
    float fX = reinterpret_cast<float&>(x);

    if (SvmMemory::mapRAM(sinOut, sizeof *sinOut))
        *sinOut = sinf(fX);
    if (SvmMemory::mapRAM(cosOut, sizeof *cosOut))
        *cosOut = cosf(fX);
}

uint64_t _SYS_shl_i64(uint32_t aL, uint32_t aH, uint32_t b)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    return a << b;
}

uint64_t _SYS_srl_i64(uint32_t aL, uint32_t aH, uint32_t b)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    return a >> b;
}

int64_t _SYS_sra_i64(uint32_t aL, uint32_t aH, uint32_t b)
{
    int64_t a = aL | (uint64_t)aH << 32;
    return a >> b;
}

uint64_t _SYS_mul_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return a * b;
}

int64_t _SYS_sdiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    int64_t a = aL | (uint64_t)aH << 32;
    int64_t b = bL | (uint64_t)bH << 32;
    return b ? (a / b) : 0;
}

uint64_t _SYS_udiv_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return b ? (a / b) : 0;
}

int64_t _SYS_srem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    int64_t a = aL | (uint64_t)aH << 32;
    int64_t b = bL | (uint64_t)bH << 32;
    return b ? (a % b) : a;
}

uint64_t _SYS_urem_i64(uint32_t aL, uint32_t aH, uint32_t bL, uint32_t bH)
{
    uint64_t a = aL | (uint64_t)aH << 32;
    uint64_t b = bL | (uint64_t)bH << 32;
    return b ? (a % b) : a;
}


}  // extern "C"
