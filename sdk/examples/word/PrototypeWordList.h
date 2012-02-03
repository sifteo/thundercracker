#ifndef PROTOTYPEWORDLIST_H
#define PROTOTYPEWORDLIST_H

#include <sifteo.h>

class PrototypeWordList
{
public:
    PrototypeWordList();
    static bool pickWord(char* buffer);
    static bool isWord(const char* string, bool& isCommon);
    static bool bitsToString(uint32_t bits, char* buffer);

private:
    //static const char* sList[];
};

#endif // PROTOTYPEWORDLIST_H
