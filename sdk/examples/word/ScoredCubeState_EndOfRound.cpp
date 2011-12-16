#include "ScoredCubeState_EndOfRound.h"

#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_EndOfRound.h"
#include "SavedData.h"

unsigned ScoredCubeState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
    case EventID_Paint:
        paint();
        break;

    case EventID_NewAnagram:
        return CubeStateIndex_NotWordScored;

    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned ScoredCubeState_EndOfRound::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void ScoredCubeState_EndOfRound::paint()
{
    Cube& c = getStateMachine().getCube();
    // FIXME vertical words
    bool neighbored =
            (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
            c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED);
    const Sifteo::AssetImage& bg =
         neighbored ?
            BGNotWordConnected :
            BGNotWordNotConnected;
    VidMode_BG0 vid(c.vbuf);
    vid.init();
    if (GameStateMachine::getTime() <= TEETH_ANIM_LENGTH)
    {
        vid.BG0_drawAsset(Vec2(0,0), bg);
        paintLetters(vid, (neighbored ? FontNeighbored : FontUnneighbored));
        char string[5];
        sprintf(string, "%d", GameStateMachine::getSecondsLeft());
    #if DEBUGZZZZZZZZZ
        printf("%d %s\n", getStateMachine().getCube().id(), string);
    #endif
        vid.BG0_text(Vec2(5 + (4 - strlen(string)), 14), FontSmall, string);
        paintTeeth(vid, true, false);
        return;
    }

    switch (getStateMachine().getCube().id())
    {
    case 0:
        vid.BG0_drawAsset(Vec2(0,0), BGNotWordNotConnected);
        vid.BG0_text(Vec2(5,4), FontSmall, "Score");
        char string[17];
        sprintf(string, "%.5d", GameStateMachine::getScore());
        vid.BG0_text(Vec2(5,6), FontSmall, string);
        break;

        // TODO high scores
    case 1:
        vid.BG0_drawAsset(Vec2(0,0), StartScreen);
        break;

    default:
        vid.BG0_drawAsset(Vec2(0,0), BGNotWordNotConnected);
        vid.BG0_text(Vec2(3,4), FontSmall, "High Scores");

        for (unsigned i = arraysize(SavedData::sHighScores) - 1;
             i >= 0;
             --i)
        {
            if (SavedData::sHighScores[i] == 0)
            {
                break;
            }
            char string[17];
            sprintf(string, "%.5d", SavedData::sHighScores[i]);
            vid.BG0_text(Vec2(5,4 + (arraysize(SavedData::sHighScores) - i) * 2),
                         FontSmall,
                         string);
        }
        break;
    }
}
