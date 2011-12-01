#include "Dictionary.h"
#include <string.h>
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;

Dictionary::Dictionary()
{
}

const char* Dictionary::pickWord(unsigned length)
{
    // TODO
    return PrototypeWordList::pickWord(length);
}

bool Dictionary::isWord(const char* string)
{
    return PrototypeWordList::isWord(string);
}


