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
    "ROYAL",
    "WITHIN",
    "DANCE",
    "BESIDE",
    "UNLESS",
    "MIDDLE",
    "VESSEL",
    "TERMS",
    "WHOLE",
    "TURNED",
    "MEMBER",
    "SECOND",
    "FIGHT",
    "WRONG",
    "NOTICE",
    "OCCUPY",
    "BECOME",
    "AFFAIR",
    "WINDOW",
};

const static unsigned char pickAnagrams[] =
{
    2,	// ROYAL, uncommon anagrams: 0
    3,	// WITHIN, uncommon anagrams: 0
    2,	// DANCE, uncommon anagrams: 0
    2,	// BESIDE, uncommon anagrams: 0
    2,	// UNLESS, uncommon anagrams: 0
    2,	// MIDDLE, uncommon anagrams: 0
    2,	// VESSEL, uncommon anagrams: 0
    3,	// TERMS, uncommon anagrams: 0
    4,	// WHOLE, uncommon anagrams: 0
    2,	// TURNED, uncommon anagrams: 0
    2,	// MEMBER, uncommon anagrams: 0
    2,	// SECOND, uncommon anagrams: 0
    2,	// FIGHT, uncommon anagrams: 0
    2,	// WRONG, uncommon anagrams: 0
    2,	// NOTICE, uncommon anagrams: 0
    2,	// OCCUPY, uncommon anagrams: 0
    2,	// BECOME, uncommon anagrams: 0
    2,	// AFFAIR, uncommon anagrams: 0
    2,	// WINDOW, uncommon anagrams: 0
};

Dictionary::Dictionary()
{
}

bool Dictionary::pickWord(char* buffer, unsigned& numAnagrams)
{
    ASSERT(buffer);

    if (GameStateMachine::getCurrentMaxLettersPerCube() > 1)
    {
        STATIC_ASSERT(arraysize(pickAnagrams) >= arraysize(picks));
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

bool Dictionary::trim(const char* word, char* buffer)
{
    ASSERT(word);
    ASSERT(buffer);
    int firstLetter;
    int wordLen = _SYS_strnlen(word, MAX_LETTERS_PER_WORD + 1);
    for (firstLetter = 0; firstLetter < wordLen; ++firstLetter)
    {
        if (word[firstLetter] >= 'A' && word[firstLetter] <= 'Z')
        {
            break;
        }
    }
    ASSERT(firstLetter < wordLen);
    int lastLetter;
    for (lastLetter = wordLen - 1; lastLetter >= firstLetter; --lastLetter)
    {
        if (word[lastLetter] >= 'A' && word[lastLetter] <= 'Z')
        {
            break;
        }
    }
    ASSERT(lastLetter >= 0);
    ASSERT(lastLetter >= firstLetter);

    int i;
    for (i = firstLetter; i <= lastLetter; ++i)
    {
        buffer[i - firstLetter] = word[i];
    }
    buffer[i] = '\0';
    ASSERT(_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) > 0);
    ASSERT((int)_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) <= wordLen);
    return (firstLetter < wordLen && lastLetter >= 0 && lastLetter >= firstLetter);
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
