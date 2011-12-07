#include "Dictionary.h"
#include <string.h>
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;
#include <stdio.h>
#include "EventID.h"
#include "EventData.h"

char Dictionary::sOldWords[MAX_OLD_WORDS][MAX_LETTERS_PER_WORD + 1];
unsigned Dictionary::sNumOldWords = 0;

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

bool Dictionary::isOldWord(const char* word)
{
    for (unsigned i = 0; i < sNumOldWords; ++i)
    {
        if (strcmp(sOldWords[i], word) == 0)
        {
            return true;
        }
    }
    return false;
}


void Dictionary::sOnEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewAnagram:
        sNumOldWords = 0;
        break;

    case EventID_NewWordFound:
        if (sNumOldWords + 1 < MAX_OLD_WORDS)
        {
            strcpy(sOldWords[sNumOldWords++], data.mWordFound.mWord);
        }
        break;

    default:
        break;
    }
}
