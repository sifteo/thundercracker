#include "Dictionary.h"
#include <string.h>
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;
#include <stdio.h>
#include "EventID.h"
#include "EventData.h"
#include "WordGame.h"

char Dictionary::sOldWords[MAX_OLD_WORDS][MAX_LETTERS_PER_WORD + 1];
unsigned Dictionary::sNumOldWords = 0;
unsigned Dictionary::sRandSeed = 0;
const unsigned WORD_RAND_SEED_INCREMENT = 1000;

Dictionary::Dictionary()
{
}

const char* Dictionary::pickWord(unsigned length)
{
    // TODO remove demo code
    sRandSeed += WORD_RAND_SEED_INCREMENT;
    WordGame::seedRand(sRandSeed++);

    //return "CITIES";
    const char* word = PrototypeWordList::pickWord(length);
    LOG(("picked word %s\n", word));
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

    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_EndOfRoundScored:
            sRandSeed += 120 * WORD_RAND_SEED_INCREMENT;
            break;
        }
        break;

    default:
        break;
    }
}
