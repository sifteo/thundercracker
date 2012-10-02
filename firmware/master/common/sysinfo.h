#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>

/*
 * General system information.
 */
namespace SysInfo {
    extern const void* UniqueId;
    static const unsigned UniqueIdNumBytes = 12;

#ifdef SIFTEO_SIMULATOR
    void init();
#endif
}

#endif // SYSINFO_H
