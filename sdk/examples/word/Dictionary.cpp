#include "Dictionary.h"
#include <string.h>
#include "TrieNode.h"

extern TrieNode root;

Dictionary::Dictionary()
{
}

const char* Dictionary::pickWord(unsigned length)
{
    // TODO
    return "Fredaz";
}

bool Dictionary::isWord(const char* string)
{
    return root.findWord(string);
}


