#include "Dictionary.h"
#include <string.h>
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;
#include <stdio.h>

Dictionary::Dictionary()
{
}

const char* Dictionary::pickWord(unsigned length)
{
    // TODO
    const char* word = PrototypeWordList::pickWord(length);
#ifdef DEBUG
    printf("picked word %s\n", word);
#endif
    return word;
}

bool Dictionary::isWord(const char* string)
{
    return PrototypeWordList::isWord(string);
}


