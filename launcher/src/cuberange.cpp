/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker launcher
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

#include "cuberange.h"
#include <sifteo.h>
using namespace Sifteo;


CubeRange::CubeRange(const _SYSMetadataCubeRange *cr)
{
    if (cr)
        sys = *cr;
    else
        bzero(sys);
}

bool CubeRange::isEmpty()
{
    return sys.minCubes == 0 && sys.maxCubes == 0;
}

bool CubeRange::isValid()
{
    return sys.minCubes <= sys.maxCubes && sys.minCubes <= _SYS_NUM_CUBE_SLOTS;
}

bool CubeRange::isSatisfied(const CubeSet &cs)
{
    unsigned count = cs.count();
    return sys.minCubes <= count && count <= sys.maxCubes;
}

void CubeRange::set()
{
    LOG("LAUNCHER: Using cube range [%d,%d]\n", sys.minCubes, sys.maxCubes);
    System::setCubeRange(sys.minCubes, sys.maxCubes);
}
