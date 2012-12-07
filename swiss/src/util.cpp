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
