#include "securerandom.h"

#include <stdint.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#endif


bool SecureRandom::generate(uint8_t *buffer, unsigned len)
{
#ifdef _WIN32

    bool success = false;
    HMODULE lib = LoadLibrary("ADVAPI32.DLL");
    if (lib) {
        typedef BOOLEAN(APIENTRY *RtlGenRandom_t)(void*, ULONG);
        RtlGenRandom_t fn = (RtlGenRandom_t) GetProcAddress(lib, "SystemFunction036");
        success = fn && fn(buffer, len);
    }
    return success;

#else

    FILE *f = fopen("/dev/random", "rb");
    if (!f)
        return false;

    bool success = fread(buffer, len, 1, f) == 1;

    fclose(f);
    return success;

#endif
}
