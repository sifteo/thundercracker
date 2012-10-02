
#include "sysinfo.h"
#include <stdlib.h>
#include "ostime.h"

namespace SysInfo {

    static uint8_t uidBytes[UniqueIdNumBytes];

    void init()
    {
        uint32_t *p = reinterpret_cast<uint32_t*>(&uidBytes[0]);

        for (unsigned i = 0; i < UniqueIdNumBytes / sizeof(uint32_t); ++i) {

            // ultra cheeseball. will (obviously) not be constant between
            // runs of the simulator
            *p = rand() ^ rand() ^ uint32_t(OSTime::clock() * 1e6);
            p++;
        }
    }

    const void* UniqueId = reinterpret_cast<void*>(&uidBytes[0]);

}
