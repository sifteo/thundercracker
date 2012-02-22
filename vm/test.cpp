#include <sifteo.h>
using namespace Sifteo;

void siftmain()
{
    while (1) {
        String<63> str;
        uint16_t battery = 5;
        _SYS_getRawBatteryV(0, (uint16_t*) &battery);
        str << "Battery: " << battery;
    }
}
