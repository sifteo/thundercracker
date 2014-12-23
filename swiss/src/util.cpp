/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * swiss - your Sifteo utility knife
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

#include "util.h"

#include <stdlib.h>
#include <string.h>

namespace Util {

bool parseVolumeCode(const char *str, unsigned &code)
{
    /*
     * Volume codes are 8-bit hexadecimal strings.
     * Returns true and fills in 'code' if valid.
     */

    if (!*str)
        return false;

    char *end;
    code = strtol(str, &end, 16);
    return !*end && code < 0x100;
}

const char *filepathBase(const char *path)
{
    /*
     * Find the base element of the filepath - ie, anything beyond
     * the last separator, including both the filename and suffix.
     */

    // look for standard separator first
    const char *p = strrchr(path, '/');
    if (p == NULL) {

        // fall back to windows separator
        p = strrchr(path, '\\');

        // no separator
        if (p == NULL) {
            return path;
        }
    }

    // step past the separator found by strrchr
    // we've already returned above if there's no separator
    return p + 1;
}

} // namespace Util
