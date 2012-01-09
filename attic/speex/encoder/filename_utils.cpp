
#include "filename_utils.h"
#include <string.h>

using namespace std;

// get the containing directory for a file
string FileNameUtils::directoryPath(const char *filepath)
{
    const char *p = strrchr(filepath, '/');
    if (p == NULL) {
        return "";
    }
    
    return string(filepath, p - filepath);
}

// base file name, with no path & no suffix
string FileNameUtils::baseName(const string &filepath)
{
    size_t d = filepath.find_last_of('/');
    if (d == string::npos) { // if no slash, assume the beginning
        d = 0;
    }
    else { // otherwise step past the slash
        d++;
    }
    
    size_t p = filepath.find_last_of('.');
    if (p == string::npos) {
        return "";
    }
    
    return string(filepath, d, p - d);
}

// stolen from stir
string FileNameUtils::guardName(const char *filepath)
{
    char c;
    char prev = '_';
    string guard(1, prev);

    while ((c = *filepath)) {
        c = toupper(c);

        if (isalpha(c)) {
            prev = c;
            guard += prev;
        } else if (prev != '_') {
            prev = '_';
            guard += prev;
        }

        filepath++;
    }
    return guard;
}
