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
unsigned Dictionary::sRound = 0;
unsigned Dictionary::sPickIndex = 0;
const unsigned WORD_RAND_SEED_INCREMENT = 88;
const unsigned DEMO_MAX_DETERMINISTIC_ROUNDS = 5;
const static char* picks[] =
{
    "ARREST",
    "STREET",
    "STREAM",
    "DESIRE",
    "SISTER",
    "RELIEF",
    "ALMOST",
    "AGREED",
    "LISTEN",
    "ESCAPE",
    "DEBATE",
    "SECURE",
    "LIFTED",
    "PEOPLE",
    "RECENT",
    "REPORT",
    "DESIGN",
    "ENTIRE",
    "REMAIN",
    "SPREAD",
    "FOREST",
    "REPAIR",
    "OBTAIN",
    "SINGLE",
    "ITSELF",
    "SELECT",
    "ENGAGE",
    "GARDEN",
    "DOCTOR",
    "REALLY",
    "ASSURE",
    "INCHES",
    "REFUSE",
    "RESULT",
    "SENATE",
    "CALLED",
    "CHARGE",
    "SLOWER",
    "SMILED",
    "SIMPLE",
    "MODERN",
    "DECIDE",
    "PROPER",
    "SUFFER",
    "MATTER",
    "SILENT",
    "RATHER",
    "ROLLED",
    "MOTION",
    "INFORM",
    "MOTHER",
    "PICKED",
    "FILLED",
    "DIVIDE",
    "INDEED",
    "ENERGY",
    "SUMMER",
    "PERIOD",
    "DRIVEN",
    "PUSHED",
    "AFRAID",
    "LESSON",
    "NATION",
    "RECORD",
    "ARRIVE",
    "INSIDE",
    "BOTTOM",
    "FOURTH",
    "INCOME",
    "STRUCK",
};

Dictionary::Dictionary()
{
}

bool Dictionary::pickWord(char* buffer)
{
    ASSERT(buffer);
    //strcpy(buffer, "WONDER");
    //return true;

    strcpy(buffer, picks[sPickIndex]);
    sPickIndex = (sPickIndex + 1) % arraysize(picks);
    return true;
    if (PrototypeWordList::pickWord(buffer))
    {
        DEBUG_LOG(("picked word %s\n", buffer));
        return true;
    }
    return false;
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
            // TODO remove demo code
            // start the first 5 rounds off with a set word list (reset the
            // rand to a value that is dependent on the round, but then increments)
            // randomize based on time from here on out
            int64_t nanosec;
            _SYS_ticks_ns(&nanosec);
            WordGame::seedRand((unsigned)nanosec);
            break;
        }
        break;

    default:
        break;
    }
}
