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

#pragma once
#include <sifteo.h>

class DefaultCubeResponder
{
public:
    void init(Sifteo::CubeID cid);
    void init();
    void paint();

    static unsigned callCount() { return called; }
    static void resetCallCount() { called = 0; }

private:
    Sifteo::Short2 position;
    Sifteo::Short2 velocity;
    Sifteo::CubeID cube;
    bool wasTouching;

    static unsigned called;

    void motionUpdate();

    Sifteo::Int2 fpRound(Sifteo::Int2 fp, unsigned bits) {
        return (fp + (Sifteo::vec(1,1) << (bits-1))) >> bits;
    }

    Sifteo::Int2 fpTrunc(Sifteo::Int2 fp, unsigned bits) {
        return fp >> bits;
    }
};
