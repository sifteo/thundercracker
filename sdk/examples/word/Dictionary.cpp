#include "Dictionary.h"
#include <string.h>

Dictionary::Dictionary()
{
}

const char* Dictionary::pickWord(unsigned length)
{
    // TODO
    return "zephyr";
}

bool Dictionary::isWord(const char* string)
{
    // TODO
    return strcmp(string, pickWord(strlen(string))) == 0;
}
