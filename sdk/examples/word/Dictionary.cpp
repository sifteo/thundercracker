#include "Dictionary.h"
//#include "TrieNode.h"
#include "PrototypeWordList.h"
//extern TrieNode root;
#include "EventID.h"
#include "EventData.h"
#include "WordGame.h"
#include "DictionaryData.h"

WordID Dictionary::sPossibleWordIDs[MAX_WORDS_PER_PUZZLE];
bool Dictionary::sPossibleWordFound[MAX_WORDS_PER_PUZZLE];
unsigned Dictionary::sNumPossibleWords = 0;
unsigned Dictionary::sRandSeed = 0;
unsigned Dictionary::sRound = 0;
int Dictionary::sPuzzleIndex = -1; // start at -1, or invalid
const unsigned WORD_RAND_SEED_INCREMENT = 88;
const unsigned DEMO_MAX_DETERMINISTIC_ROUNDS = 5;


Dictionary::Dictionary()
{
}

bool Dictionary::getNextPuzzle(char* buffer,
                          unsigned char& numAnagrams,
                          unsigned char& leadingSpaces,
                          unsigned char& maxLettersPerCube)
{
    ASSERT(buffer);
    sPuzzleIndex = (sPuzzleIndex + 1) % NUM_PUZZLES;
    if (!getPuzzle(sPuzzleIndex,
                   buffer,
                   numAnagrams,
                   leadingSpaces,
                   maxLettersPerCube))
    {
        return false;
    }

    sNumPossibleWords = puzzlesNumPossibleAnagrams[sPuzzleIndex];
    for (unsigned i = 0; i < sNumPossibleWords; ++i)
    {
        sPossibleWordIDs[i] = puzzlesPossibleWordIndexes[sPuzzleIndex][i];
        sPossibleWordFound[i] = false;
    }

    return true;

    /* TODO move to another function if (PrototypeWordList::getNextPuzzle(buffer))
    {
        DEBUG_LOG(("picked word %s\n", buffer));
        return true;
    }
    return false;
    */
}

bool Dictionary::getPuzzle(int index,
                           char* buffer,
                           unsigned char& numAnagrams,
                           unsigned char& leadingSpaces,
                           unsigned char& maxLettersPerCube)
{
    ASSERT(buffer);
    ASSERT(index >= 0);
    ASSERT(index < (int)arraysize(puzzles));
    _SYS_strlcpy(buffer, puzzles[index], MAX_LETTERS_PER_WORD + 1);

    numAnagrams = MAX(1, puzzlesNumGoalAnagrams[index]);
    // TODO how many leading spaces?
    leadingSpaces = puzzlesNumLeadingSpaces[index];

    // TODO data-driven
    maxLettersPerCube =
            (_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) > 6) ? 3 : 2;

    // update for next pick
    return true;
}

int Dictionary::getPuzzleIndex()
{
    // TODO data-driven
    return sPuzzleIndex;
}

bool Dictionary::shouldScrambleCurrentWord()
{
    return sPuzzleIndex < 0 || sPuzzleIndex >= (int)arraysize(puzzlesScramble) ?
                true : puzzlesScramble[sPuzzleIndex];
}

bool Dictionary::findNextSolutionWordPieces(unsigned maxPieces,
                                           unsigned maxLettersPerPiece,
                                            char wordPieces[][MAX_LETTERS_PER_CUBE])
{
    for (unsigned i = 0; i < sNumPossibleWords; ++i)
    {
        if (!sPossibleWordFound[i])
        {
            char word[MAX_LETTERS_PER_WORD + 1];
            WordID wid = sPossibleWordIDs[i];
            ASSERT(wid >=0);
            if (getWordFromID(wid, word))
            {
                return getSolutionPieces(word, maxPieces, maxLettersPerPiece, wordPieces);
            }
        }
    }
    return false;
}

bool Dictionary::getCurrentPieces(unsigned maxPieces,
                                  unsigned maxLettersPerPiece,
                                  char puzzlePieces[][MAX_LETTERS_PER_CUBE])
{
    if (sPuzzleIndex < 0)
    {
        return false;
    }
    // add any leading and/or trailing spaces to odd-length words
    char spacesAdded[MAX_LETTERS_PER_WORD + 1];
    _SYS_memset8((uint8_t*)spacesAdded, 0, sizeof(spacesAdded));

    const char *word = puzzles[sPuzzleIndex];
    unsigned wordLen = _SYS_strnlen(word, MAX_LETTERS_PER_WORD + 1);
    ASSERT(GameStateMachine::getCurrentMaxLettersPerWord() >= wordLen);
    unsigned numBlanks =
            GameStateMachine::getCurrentMaxLettersPerWord() - wordLen;
    unsigned leadingBlanks = puzzlesNumLeadingSpaces[sPuzzleIndex];
    if (numBlanks == 0)
    {
        _SYS_strlcpy(spacesAdded, word, sizeof spacesAdded);
    }
    else
    {
        unsigned i;
        for (i = 0;
             i < GameStateMachine::getCurrentMaxLettersPerWord();
             ++i)
        {
            if (i < leadingBlanks || i >= leadingBlanks + wordLen)
            {
                spacesAdded[i] = ' ';
            }
            else
            {
                spacesAdded[i] = word[i - leadingBlanks];
            }
        }
        spacesAdded[i] = '\0';
    }

    for (unsigned i=0; i < maxPieces; ++i)
    {
        for (unsigned j=0; j < maxLettersPerPiece; ++j)
        {
            puzzlePieces[i][j] = spacesAdded[j + i * maxLettersPerPiece];
        }
    }
    return true;
}

bool Dictionary::getSolutionPieces(const char *word,
                               unsigned maxPieces,
                               unsigned maxLettersPerPiece,
                               char wordPieces[][MAX_LETTERS_PER_CUBE])
{
    // add any leading and/or trailing spaces to odd-length words
    char spacesAdded[MAX_LETTERS_PER_WORD + 1];
    unsigned maxLettersPerWord = maxPieces * maxLettersPerPiece;
    unsigned wordLen = _SYS_strnlen(word, arraysize(spacesAdded));
    unsigned numBlanks = maxLettersPerWord - wordLen;
    unsigned numSolutionPieces = wordLen/maxLettersPerPiece;
    if ((wordLen % maxLettersPerPiece) != 0)
    {
        ++numSolutionPieces;
    }

    if (numSolutionPieces < maxPieces)
    {
        for (unsigned i=0; i < maxPieces; ++i)
        {
            for (unsigned j=0; j < maxLettersPerPiece; ++j)
            {
                wordPieces[i][j] = ' ';
            }
        }
    }
    char mainPuzzlePieces[NUM_CUBES][MAX_LETTERS_PER_CUBE];
    bool success = getCurrentPieces(maxPieces, maxLettersPerPiece, mainPuzzlePieces);
    ASSERT(success);

    for (unsigned leadingBlanks = 0; leadingBlanks <= numBlanks; ++leadingBlanks)
    {
        // create a possible configuration for the letters in a row for the solution
        unsigned i;
        for (i = 0; i < maxLettersPerWord; ++i)
        {
            if (i < leadingBlanks || i >= leadingBlanks + wordLen)
            {
                spacesAdded[i] = ' ';
            }
            else
            {
                spacesAdded[i] = word[i - leadingBlanks];
            }
        }
        spacesAdded[i] = '\0';

        // test if the solution configuration is possible from the whole
        // set of puzzle pieces
        unsigned usedPieceCount = 0;
        bool usedPiece[NUM_CUBES];
        for (i = 0; i < arraysize(usedPiece); ++i)
        {
            usedPiece[i] = false;
        }

        // nesting a bunch of tiny loops here... I can move it offline,
        // but trading CPU to save memory

        // for each solution cube/piece
        for (unsigned c = 0; c < numSolutionPieces; ++c)
        {
            // look through all the main puzzle pieces/cubes
            for (unsigned cm = 0; cm < maxPieces; ++cm)
            {
                if (usedPiece[cm])
                {
                    continue;
                }
                bool matchedAllLetters = true;
                // try all possible shifts of cube letters
                for (unsigned s = 0; s < maxLettersPerPiece; ++s)
                {
                    matchedAllLetters = true;
                    // check all letters
                    for (unsigned l = 0; l < maxLettersPerPiece; ++l)
                    {
                        // save this configuration out, in case it is correct
                        if (mainPuzzlePieces[cm][(l + s) % maxLettersPerPiece] !=
                            spacesAdded[l + c * maxLettersPerPiece])
                        {
                            matchedAllLetters = false;
                            break;
                        }
                        wordPieces[cm][l] = spacesAdded[l + c * maxLettersPerPiece];
                    }
                    if (matchedAllLetters)
                    {
                        break;
                    }
                }

                if (matchedAllLetters)
                {
                    usedPiece[cm] = true;
                    ++usedPieceCount;
                    break;
                }
            }
        }

        if (usedPieceCount == numSolutionPieces)
        {
            return true;
        }
    }
    ASSERT(0); // failed to match puzzle pieces to possible word

    return false;
}

WordID Dictionary::findWordID(const char* word)
{
    WordID wordID = WORD_ID_NONE;
    bool isBonus;
    PrototypeWordList::isWord(word, isBonus, wordID);
    return wordID;
}

bool Dictionary::getWordFromID(WordID wid, char *buffer)
{
    return PrototypeWordList::getWordFromID(wid, buffer);
}

bool Dictionary::isWord(const char* string, bool& isBonus)
{
    WordID wordID = WORD_ID_NONE;
    return PrototypeWordList::isWord(string, isBonus, wordID);
}

bool Dictionary::isOldWord(const char* word)
{
    WordID wid = findWordID(word);
    ASSERT(wid != WORD_ID_NONE);
    for (unsigned i = 0; i < sNumPossibleWords; ++i)
    {
        if (wid == sPossibleWordIDs[i] && sPossibleWordFound[i])
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
    buffer[i- firstLetter] = '\0';
    ASSERT(_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) > 0);
    ASSERT((int)_SYS_strnlen(buffer, MAX_LETTERS_PER_WORD + 1) <= wordLen);
    return (firstLetter < wordLen && lastLetter >= 0 && lastLetter >= firstLetter);
}

unsigned char Dictionary::getPuzzleMetaLetterIndex(int puzzleIndex)
{
    return puzzleIndex < 0 || puzzleIndex >= (int)arraysize(puzzlesMetaLetterIndex) ?
                0 : puzzlesMetaLetterIndex[puzzleIndex];
}

unsigned char Dictionary::getCurrentPuzzleMetaLetterIndex()
{
    return getPuzzleMetaLetterIndex(sPuzzleIndex);
}


bool Dictionary::isMetaPuzzle(int index)
{
    return getPuzzleMetaLetterIndex(index) == 255;
}

bool Dictionary::currentIsMetaPuzzle()
{
    return isMetaPuzzle(sPuzzleIndex);
}

bool Dictionary::currentStartsNewMetaPuzzle()
{
    // TODO
    return sPuzzleIndex == 0 || isMetaPuzzle(sPuzzleIndex - 1);
}

bool Dictionary::getMetaPuzzle(char *buffer,
                               unsigned char &leadingSpaces,
                               unsigned char &maxLettersPerCube)
{
    for (int m = sPuzzleIndex; m < (int)NUM_PUZZLES; ++m)
    {
        if (isMetaPuzzle(m))
        {
            unsigned char numAnagrams;
            return getPuzzle(m, buffer, numAnagrams, leadingSpaces, maxLettersPerCube);
        }
    }
    return false;
}

char Dictionary::getMetaLetter()
{
    unsigned char i = getCurrentPuzzleMetaLetterIndex();
    char word[MAX_LETTERS_PER_WORD + 1];
    unsigned char a, b, c;
    if (getPuzzle(sPuzzleIndex, word, a, b, c))
    {
        return word[i];
    }
    ASSERT(0);
    return ' ';
}

void Dictionary::sOnEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_NewPuzzle:
        for (unsigned i = 0; i < arraysize(sPossibleWordIDs); ++i)
        {
            // TODO get all the possible words
            sPossibleWordFound[i] = false;
        }
        break;

    case EventID_NewWordFound:
        {
            WordID wid = findWordID(data.mWordFound.mWord);
            ASSERT(wid != WORD_ID_NONE);
            for (unsigned i = 0; i < arraysize(sPossibleWordIDs); ++i)
            {
                if (wid == sPossibleWordIDs[i])
                {
                    sPossibleWordFound[i] = true;
                }
            }
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
