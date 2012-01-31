#include "Dictionary.h"
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;
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
    "WITHIN",
    "OCCUPY",
    "BESIDE",
    "UNLESS",
    "MIDDLE",
    "VESSEL",
    "TURNED",
    "MEMBER",
    "SECOND",
    "NOTICE",
    "BECOME",
    "AFFAIR",
    "WINDOW",

};

const static unsigned char pickAnagrams[] =
{
    3,	// WITHIN, uncommon anagrams: 0
    2,	// OCCUPY, uncommon anagrams: 0
    2,	// BESIDE, uncommon anagrams: 0
    2,	// UNLESS, uncommon anagrams: 0
    2,	// MIDDLE, uncommon anagrams: 0
    2,	// VESSEL, uncommon anagrams: 0
    2,	// TURNED, uncommon anagrams: 0
    2,	// MEMBER, uncommon anagrams: 0
    2,	// SECOND, uncommon anagrams: 0
    2,	// NOTICE, uncommon anagrams: 0
    2,	// BECOME, uncommon anagrams: 0
    2,	// AFFAIR, uncommon anagrams: 0
    2,	// WINDOW, uncommon anagrams: 0
    3,	// DURING, uncommon anagrams: 1
    2,	// HAPPEN, uncommon anagrams: 1
    4,	// MOTION, uncommon anagrams: 1
    2,	// OFFICE, uncommon anagrams: 1
    2,	// JUMPED, uncommon anagrams: 1
    2,	// THOUGH, uncommon anagrams: 1
    2,	// EXCEPT, uncommon anagrams: 1
    2,	// ANSWER, uncommon anagrams: 1
    2,	// VALLEY, uncommon anagrams: 1
    3,	// FIGURE, uncommon anagrams: 1
    2,	// TICKET, uncommon anagrams: 1
    2,	// INJURE, uncommon anagrams: 1
    2,	// REGION, uncommon anagrams: 1
    2,	// BRIGHT, uncommon anagrams: 1
    3,	// UNABLE, uncommon anagrams: 1
    3,	// RETURN, uncommon anagrams: 1
    2,	// DIRECT, uncommon anagrams: 1
    2,	// ACCEPT, uncommon anagrams: 1
    2,	// FAMOUS, uncommon anagrams: 1
    2,	// AVENUE, uncommon anagrams: 1
    3,	// BELONG, uncommon anagrams: 1
    2,	// FATHER, uncommon anagrams: 1
    2,	// OBLIGE, uncommon anagrams: 1
    3,	// PASSED, uncommon anagrams: 1
    3,	// FINISH, uncommon anagrams: 1
    2,	// AROUND, uncommon anagrams: 1
    2,	// REASON, uncommon anagrams: 1
    2,	// FOLLOW, uncommon anagrams: 1
    3,	// CAREER, uncommon anagrams: 1
    3,	// SPRING, uncommon anagrams: 1
    2,	// BROKEN, uncommon anagrams: 1
    3,	// CORNER, uncommon anagrams: 1
    2,	// PLAINS, uncommon anagrams: 1
    2,	// PROMPT, uncommon anagrams: 1
    2,	// WEIGHT, uncommon anagrams: 1
    3,	// RETIRE, uncommon anagrams: 1
    2,	// ACTION, uncommon anagrams: 1
    3,	// WINTER, uncommon anagrams: 1
    2,	// POUNDS, uncommon anagrams: 1
    3,	// ISLAND, uncommon anagrams: 1
    2,	// ACROSS, uncommon anagrams: 1
    2,	// DEGREE, uncommon anagrams: 1
    2,	// NUMBER, uncommon anagrams: 1
    2,	// YELLOW, uncommon anagrams: 1
    2,	// SUDDEN, uncommon anagrams: 1
    3,	// EMPIRE, uncommon anagrams: 1
    2,	// EMPLOY, uncommon anagrams: 1
    3,	// STRING, uncommon anagrams: 1
    2,	// DOLLAR, uncommon anagrams: 1
    2,	// HEIGHT, uncommon anagrams: 1
    3,	// NEARLY, uncommon anagrams: 1
    2,	// OXYGEN, uncommon anagrams: 1
    4,	// PUSHED, uncommon anagrams: 1
    2,	// MONTHS, uncommon anagrams: 1
    2,	// SYSTEM, uncommon anagrams: 1
    3,	// BECAME, uncommon anagrams: 1
    3,	// TOWARD, uncommon anagrams: 1
    3,	// ENGINE, uncommon anagrams: 2
    4,	// SUFFER, uncommon anagrams: 2
    3,	// EFFORT, uncommon anagrams: 2
    4,	// DIVIDE, uncommon anagrams: 2
    3,	// MINUTE, uncommon anagrams: 2
    3,	// PERSON, uncommon anagrams: 2
    4,	// INCOME, uncommon anagrams: 2
    3,	// PREFER, uncommon anagrams: 2
    6,	// DESIGN, uncommon anagrams: 2
    3,	// INTEND, uncommon anagrams: 2
    3,	// DESERT, uncommon anagrams: 2
    3,	// SALARY, uncommon anagrams: 2
    3,	// ALLEGE, uncommon anagrams: 2
    3,	// SUMMON, uncommon anagrams: 2
    3,	// WHEELS, uncommon anagrams: 2
    4,	// FILLED, uncommon anagrams: 2
    3,	// CITIES, uncommon anagrams: 2
    3,	// COURSE, uncommon anagrams: 2
    3,	// SLOWLY, uncommon anagrams: 2
    4,	// FOURTH, uncommon anagrams: 2
    3,	// PULLED, uncommon anagrams: 2
    4,	// SUMMER, uncommon anagrams: 2
    3,	// JOINED, uncommon anagrams: 2
    3,	// MOMENT, uncommon anagrams: 2
    3,	// STRONG, uncommon anagrams: 2
    3,	// ASSIST, uncommon anagrams: 2
    3,	// MANNER, uncommon anagrams: 2
    3,	// LETTER, uncommon anagrams: 2
    4,	// LESSON, uncommon anagrams: 2
    3,	// TRAVEL, uncommon anagrams: 2
    3,	// PLEASE, uncommon anagrams: 2
    3,	// ABOARD, uncommon anagrams: 2
    3,	// EUROPE, uncommon anagrams: 2
    3,	// AUGUST, uncommon anagrams: 2
    5,	// SINGLE, uncommon anagrams: 2
    3,	// MELODY, uncommon anagrams: 2
    3,	// WONDER, uncommon anagrams: 2
    3,	// POLICE, uncommon anagrams: 2
    4,	// INFORM, uncommon anagrams: 2
    3,	// BEHIND, uncommon anagrams: 2
    4,	// MOTHER, uncommon anagrams: 2
    3,	// REGARD, uncommon anagrams: 2
    3,	// ALWAYS, uncommon anagrams: 2
    4,	// INSIDE, uncommon anagrams: 2
    5,	// REPAIR, uncommon anagrams: 3
    4,	// SLOWER, uncommon anagrams: 3
    4,	// SIMPLE, uncommon anagrams: 3
    5,	// OBTAIN, uncommon anagrams: 3
    4,	// SMILED, uncommon anagrams: 3
    4,	// PROPER, uncommon anagrams: 3
    4,	// INDEED, uncommon anagrams: 3
    4,	// ENERGY, uncommon anagrams: 3
    4,	// PERIOD, uncommon anagrams: 3
    4,	// NATION, uncommon anagrams: 3
    4,	// PICKED, uncommon anagrams: 3
    4,	// ROLLED, uncommon anagrams: 3
    5,	// SELECT, uncommon anagrams: 3
    5,	// REALLY, uncommon anagrams: 3
    4,	// AFRAID, uncommon anagrams: 3
    4,	// SILENT, uncommon anagrams: 3
    5,	// DOCTOR, uncommon anagrams: 3
    4,	// DRIVEN, uncommon anagrams: 3
    5,	// RESULT, uncommon anagrams: 3
    4,	// BOTTOM, uncommon anagrams: 3
    4,	// MODERN, uncommon anagrams: 3
    4,	// DECIDE, uncommon anagrams: 3
    6,	// RECENT, uncommon anagrams: 3
    4,	// RATHER, uncommon anagrams: 3
    4,	// ARRIVE, uncommon anagrams: 3
    5,	// ASSURE, uncommon anagrams: 3
    4,	// MATTER, uncommon anagrams: 3
    5,	// CALLED, uncommon anagrams: 3
    4,	// RECORD, uncommon anagrams: 3
    4,	// STRUCK, uncommon anagrams: 3
    6,	// PEOPLE, uncommon anagrams: 4
    5,	// ITSELF, uncommon anagrams: 4
    5,	// ENGAGE, uncommon anagrams: 4
    5,	// SENATE, uncommon anagrams: 4
    6,	// REMAIN, uncommon anagrams: 4
    5,	// CHARGE, uncommon anagrams: 4
    5,	// GARDEN, uncommon anagrams: 4
    5,	// REFUSE, uncommon anagrams: 4
    6,	// SPREAD, uncommon anagrams: 4
    6,	// ENTIRE, uncommon anagrams: 4
    6,	// FOREST, uncommon anagrams: 4
    5,	// INCHES, uncommon anagrams: 4
    7,	// ESCAPE, uncommon anagrams: 5
    6,	// LIFTED, uncommon anagrams: 5
    8,	// LISTEN, uncommon anagrams: 5
    6,	// REPORT, uncommon anagrams: 5
    8,	// ALMOST, uncommon anagrams: 5
    8,	// SISTER, uncommon anagrams: 6
    7,	// SECURE, uncommon anagrams: 6
    8,	// RELIEF, uncommon anagrams: 6
    7,	// DEBATE, uncommon anagrams: 6
    10,	// STREET, uncommon anagrams: 7
    8,	// AGREED, uncommon anagrams: 7
    9,	// STREAM, uncommon anagrams: 7
    9,	// DESIRE, uncommon anagrams: 7
    13,	// ARREST, uncommon anagrams: 11
};

Dictionary::Dictionary()
{
}

bool Dictionary::pickWord(char* buffer, unsigned& numAnagrams)
{
    ASSERT(buffer);
    //strcpy(buffer, "WONDER");
    //return true;

    if (GameStateMachine::getCurrentMaxLettersPerCube() > 1)
    {
        _SYS_strlcpy(buffer, picks[sPickIndex], GameStateMachine::getCurrentMaxLettersPerWord() + 1);

        numAnagrams = pickAnagrams[sPickIndex];
        sPickIndex = (sPickIndex + 1) % arraysize(picks);
        return true;
    }

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
        if (_SYS_strncmp(sOldWords[i], word, GameStateMachine::getCurrentMaxLettersPerWord()) == 0)
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
            _SYS_strlcpy(sOldWords[sNumOldWords++], data.mWordFound.mWord,
                         GameStateMachine::getCurrentMaxLettersPerWord() + 1);
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
                WordGame::random.seed();
            break;
        }
        break;

    default:
        break;
    }
}
