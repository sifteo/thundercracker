
#include "sysinfo.h"
#include "board.h"

namespace SysInfo {

    const void* UniqueId = reinterpret_cast<void*>(0x1FFFF7E8);

    const unsigned HardwareRev = BOARD;
}
