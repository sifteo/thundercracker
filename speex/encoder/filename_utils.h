
#ifndef _FILENAME_UTILS_H
#define _FILENAME_UTILS_H

#include <stdio.h>
#include <string>

class FileNameUtils {
public:
    static std::string directoryPath(const char *filepath);
    static std::string baseName(const std::string &filepath);
    static std::string guardName(const char *filepath);
};

#endif // _FILENAME_UTILS_H
