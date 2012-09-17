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
