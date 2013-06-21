#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdint.h>

/*
 * General system information.
 */
namespace SysInfo {

    extern const uint8_t* UniqueId;
    static const unsigned UniqueIdNumBytes = 12;

    extern const unsigned HardwareRev;

#ifdef SIFTEO_SIMULATOR
    void init();
#endif
}

#endif // SYSINFO_H
