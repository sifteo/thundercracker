#include "util.h"

#include <stdlib.h>

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

} // namespace Util
