#ifndef UTIL_H
#define UTIL_H

namespace Util {

bool parseVolumeCode(const char *str, unsigned &code);

const char *filepathBase(const char *path);

} // namespace Util

#endif // UTIL_H
