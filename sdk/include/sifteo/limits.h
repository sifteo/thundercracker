/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK
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

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

/**
 * @addtogroup macros
 * @{
 */

#ifndef CUBE_ALLOCATION
/**
 * @brief Compile-time cube limit for the current application
 *
 * These SDK headers use CUBE_ALLOCATION to size many internal arrays,
 * such as the array of loading FIFOs in AssetLoader, or the array
 * of base addresses in an AssetGroup. You 
 *
 * Every game can individually define a compile-time limit for the
 * number of supported cubes, by supplying their own value for
 * CUBE_ALLOCATION in their Makefile or with a preprocessor definition
 * that is always included above the SDK headers.
 *
 * It must be less than or equal to the firmware limit of 24.
 * Many data structures are statically allocated using this number,
 * so if a game is running low on RAM it can decrease the limit, or
 * if it needs more cubes it can be increased.
 */
#define CUBE_ALLOCATION  12
#endif

/**
 * @} end addtogroup macros
 */
