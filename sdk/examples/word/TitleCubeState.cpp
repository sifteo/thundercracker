#include "TitleCubeState.h"
#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "SavedData.h"

unsigned TitleCubeState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        paint();
        break;

    case EventID_NewRound:
        return CubeStateIndex_NotWordScored;
    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned TitleCubeState::update(float dt, float stateTime)
{
    return getStateMachine().getCurrentStateIndex();
}

void TitleCubeState::paint()
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
        vid.BG0_drawAsset(Vec2(0,0), Title);
        break;

        // TODO high scores
    case 1:
        vid.BG0_drawAsset(Vec2(0,0), StartScreen);
        break;

    default:
        vid.BG0_drawAsset(Vec2(0,0), BGNotWordNotConnected);
        /* TODO load/save
        paintNumbers(vid, Vec2(3,4), FontSmall, "High Scores");

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
            paintNumbers(vid, Vec2(5,4 + (arraysize(SavedData::sHighScores) - i) * 2),
                         FontSmall,
                         string);
        }
        */
        break;
    }
}
