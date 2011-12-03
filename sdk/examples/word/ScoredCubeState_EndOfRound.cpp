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
        paint();
        break;

    case EventID_NewAnagram:
        return ScoredCubeStateIndex_NotWord;

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
/*    const Sifteo::AssetImage& bg =
        (c.physicalNeighborAt(SIDE_LEFT) != CUBE_ID_UNDEFINED ||
         c.physicalNeighborAt(SIDE_RIGHT) != CUBE_ID_UNDEFINED) ?
            BGNewWordConnectedMiddle :
            BGNewWordConnectedLeft;
            */
    VidMode_BG0 vid(c.vbuf);
    vid.init();

    switch (getStateMachine().getCube().id())
    {
    case 0:
        vid.BG0_drawAsset(Vec2(0,0), BGNotWordNotConnected);
        vid.BG0_text(Vec2(5,4), FontSmall, "Score");
        char string[17];
        sprintf(string, "%.5d", GameStateMachine::GetScore());
        vid.BG0_text(Vec2(5,6), FontSmall, string);
        break;

        // TODO high scores
    case 1:
        vid.BG0_drawAsset(Vec2(0,0), RestartScreen);
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
