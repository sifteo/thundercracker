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
    // TODO binary search
    /*
    TrieNode* node = root;
    const char* p = string

    for(; p && *p && node; p++, node = node->children)
    {
        if (*p != node->)
    }
*/
    return false;
}


