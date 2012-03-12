#ifndef PROTOTYPEWORDLIST_H
#define PROTOTYPEWORDLIST_H

#include <sifteo.h>
#include "Dictionary.h"

class PrototypeWordList
{
public:
    PrototypeWordList();
    static bool pickWord(char* buffer);
    static bool isWord(const char* string, bool& isCommon, WordID& wordID);
    static bool getWordFromID(WordID wid, char *buffer);

    static bool bitsToString(uint64_t bits, char* buffer);

private:
    //static const char* sList[];
};

#endif // PROTOTYPEWORDLIST_H
