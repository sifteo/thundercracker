/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
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
