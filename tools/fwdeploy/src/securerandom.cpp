/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker firmware deployment tool
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

#include "securerandom.h"

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif


bool SecureRandom::generate(uint8_t *buffer, unsigned len)
{
#ifdef _WIN32

    bool success = false;
    HMODULE lib = LoadLibrary("ADVAPI32.DLL");
    if (lib) {
        typedef BOOLEAN(APIENTRY *RtlGenRandom_t)(void*, ULONG);
        RtlGenRandom_t fn = (RtlGenRandom_t) GetProcAddress(lib, "SystemFunction036");
        success = fn && fn(buffer, len);
    }
    return success;

#else

    FILE *f = fopen("/dev/random", "rb");
    if (!f)
        return false;

    bool success = fread(buffer, len, 1, f) == 1;

    fclose(f);
    return success;

#endif
}
