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

/*
 * These headers define the system Menu object.
 * To use the Menu, you must include sifteo/menu.h.
 * It is not part of the default set of headers in sifteo.h.
 */

#pragma once

/**
 * @defgroup assets Assets
 *
 * @brief Objects for loading, using, and inspecting visual and audio assets
 *
 * Audio and visual artwork packaged with a game are referred to here as _assets_.
 * This module details the ways that Sifteo Cubes store and load assets. Many of the
 * objects here are instantiated by `stir`. See the @ref asset_workflow for details.
 *
 * @{
 */

#ifndef NOT_USERSPACE
#   include <sifteo/asset/group.h>
#   include <sifteo/asset/loader.h>
#endif

#include <sifteo/asset/audio.h>
#include <sifteo/asset/image.h>

/**
 * @} endgroup menu
 */
