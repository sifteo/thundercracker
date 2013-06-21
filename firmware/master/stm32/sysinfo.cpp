
#include "sysinfo.h"
#include "board.h"

namespace SysInfo {

    const uint8_t* UniqueId = reinterpret_cast<uint8_t*>(0x1FFFF7E8);

    const unsigned HardwareRev = BOARD;
}
