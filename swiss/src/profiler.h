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

#ifndef PROFILER_H
#define PROFILER_H

#include "iodevice.h"
#include "elfdebuginfo.h"

#include <signal.h>
#include <stdio.h>
#include <map>

class Profiler
{
public:
    Profiler(IODevice &_dev);

    // entry point for the 'profile' command
    static int run(int argc, char **argv, IODevice &_dev);

    bool profile(const char *elfPath, const char *outPath);

private:
    typedef uint32_t Addr;
    typedef unsigned Count;

    enum SubSystem {
        None,
        FlashDMA,
        AudioPull,
        SVCISR,
        RFISR,
        NumSubsystems   // must be last
    };

    struct FuncInfo {
        Addr address;
        Count count;
    };

    struct Entry {
        std::string name;
        Addr address;
        Count count;

        Entry() {}
        Entry(const std::string &n, Addr a, Count c):
            name(n), address(a), count(c) {}

        // sort in descending order
        bool operator() (const Entry& a, const Entry& b) const {
            return b.count < a.count;
        }
    };

    static void onSignal(int sig);
    static void prettyPrintSamples(const std::map<Addr, Count> &addresses, uint64_t total, FILE *f);
    static const char *subSystemName(SubSystem s);

    static sig_atomic_t interruptRequested;
    static ELFDebugInfo dbgInfo;
    IODevice &dev;
};

#endif // PROFILER_H
