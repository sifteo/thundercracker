#ifndef INSPECT_H
#define INSPECT_H

#include "iodevice.h"
#include "sifteo/abi/types.h"
#include "elfdebuginfo.h"
#include "tabularlist.h"

class Inspect
{
public:
    static int run(int argc, char **argv, IODevice &_dev);

    Inspect()
    {}

    bool inspect(const char *path);

private:
    std::string uuidStr(const _SYSUUID & u);
    void dumpString(TabularList &tl, uint16_t key, const char *name);

    ELFDebugInfo dbgInfo;
};

#endif // INSPECT_H
