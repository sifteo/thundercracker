#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "CubeStateMachine.h"

typedef int WordID;
const WordID WORD_ID_NONE = -1;
const unsigned MAX_WORDS_PER_PUZZLE = 16; // as determined by offline check

class Dictionary
{
public:
    Dictionary();

    static bool pickWord(char* buffer,
                          unsigned char& numAnagrams,
                          unsigned char& leadingSpaces,
                          unsigned char& maxLettersPerCube);

    static bool findNextSolutionWordPieces(unsigned maxPieces,
                                           unsigned maxLettersPerPiece,
                                           char wordPieces[][MAX_LETTERS_PER_CUBE]);
    static bool getSolutionPieces(const char *word,
                                  unsigned maxPieces,
                                  unsigned maxLettersPerPiece,
                                  char wordPieces[][MAX_LETTERS_PER_CUBE]);
    static bool getCurrentPieces(unsigned maxPieces,
                                 unsigned maxLettersPerPiece,
                                 char puzzlePieces[][MAX_LETTERS_PER_CUBE]);

    static bool isWord(const char* string, bool& isCommon);
    static bool isOldWord(const char* word);
    static bool trim(const char* word, char* buffer);
    static int getPuzzleIndex();
    static bool doScrambleCurrentWord();
    static unsigned char getPuzzleMetaLetterIndex();
    static void sOnEvent(unsigned eventID, const EventData& data);

private:
    static WordID findWordID(const char* word);
    static bool getWordFromID(WordID wid, char *buffer);

    static WordID sPossibleWordIDs[MAX_WORDS_PER_PUZZLE];
    static bool sPossibleWordFound[MAX_WORDS_PER_PUZZLE];
    static unsigned sNumPossibleWords;
    static unsigned sRandSeed;
    static unsigned sRound;
    static int sPuzzleIndex;
};

#endif // DICTIONARY_H
