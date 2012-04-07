#ifndef SAVEDDATA_H
#define SAVEDDATA_H

#include <sifteo.h>

union EventData;
//extern const unsigned NUM_PUZZLES;
const unsigned char NUM_PUZZLE_BYTES = 40/8;//NUM_PUZZLES/8;
const unsigned char MAX_CUBES = 12;

class SavedData
{
public:
    SavedData();
    void OnEvent(unsigned eventID, const EventData& data);

    uint16_t getCurrentPuzzleIndex() const { return mCurrentPuzzleIndex; }
    uint8_t getNumHints() const { return mNumHints; }

    bool isFirstRun() const { return true; } // TODO first run detect

private:
    uint32_t mHighScores[MAX_CUBES][3];
    uint8_t mCompletedPuzzles[NUM_PUZZLE_BYTES]; // bit array
    int16_t mCurrentPuzzleIndex;
    uint8_t mNumHints;
    uint8_t mMainMenuSelection;
    uint8_t mPauseMenuSelection;
};

#endif // SAVEDDATA_H
