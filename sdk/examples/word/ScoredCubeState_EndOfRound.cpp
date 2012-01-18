#include "ScoredCubeState_EndOfRound.h"

#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "ScoredCubeState_EndOfRound.h"
#include "SavedData.h"
#include "WordGame.h"
#include "config.h"


const unsigned START_SCREEN_CUBE_INDEX = 1;
const unsigned SCORE_RHS_X = 9;
const unsigned SCORE_RHS_Y = 2;

unsigned ScoredCubeState_EndOfRound::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    case EventID_EnterState:
    case EventID_Paint:
        paint();
        break;

    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_StartOfRoundScored:
            return CubeStateIndex_StartOfRoundScored;
        }
        break;
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
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();
    WordGame::hideSprites(vid);
    if (GameStateMachine::getTime() <= TEETH_ANIM_LENGTH)
    {
        paintLetters(vid, Font1Letter);
        paintTeeth(vid, ImageIndex_Teeth, true, true);
        return;
    }
    else if (GameStateMachine::getTime() <= TEETH_ANIM_LENGTH * 2.f)
    {
        const float ANIM_LENGTH = TEETH_ANIM_LENGTH;
        const AssetImage& anim = ScorePan;
        float animTime =
                (getStateMachine().getTime() - TEETH_ANIM_LENGTH) / ANIM_LENGTH;
        animTime = MIN(animTime, 1.f);
        unsigned frame = (unsigned) (animTime * anim.frames);
        frame = MIN(frame, anim.frames - 1);

        vid.BG0_drawAsset(Vec2(0, 0), anim, frame);
        return;
    }

    switch (getStateMachine().getCube().id() - CUBE_ID_BASE)
    {
    default:
        // paint "Score" asset
        vid.BG0_drawAsset(Vec2(0,0), ScorePan, ScorePan.frames - 1);
        vid.BG0_drawAsset(Vec2(4,0), Score);
        {
            char string[17];
            sprintf(string, "%d", GameStateMachine::getScore());
            BG1Helper bg1(getStateMachine().getCube());
            paintScoreNumbers(bg1, Vec2(SCORE_RHS_X,SCORE_RHS_Y), string);
            bg1.Flush(); // TODO only flush if mask has changed recently
        }
        WordGame::instance()->setNeedsPaintSync();
        break;

    case START_SCREEN_CUBE_INDEX:
        vid.BG0_drawAsset(Vec2(0,0), ScorePan, ScorePan.frames - 1);
        {
            const float ANIM_LENGTH = 1.0f;
            const AssetImage& anim = StartPrompt;
            float animTime =
                    fmodf(getStateMachine().getTime() - 0.f, ANIM_LENGTH) / ANIM_LENGTH;
            animTime = MIN(animTime, 1.f);
            unsigned frame = (unsigned) (animTime * anim.frames);
            frame = MIN(frame, anim.frames - 1);

            BG1Helper bg1(getStateMachine().getCube());
            bg1.DrawAsset(Vec2(5, 2), anim, frame);
            bg1.Flush(); // TODO only flush if mask has changed recently
            WordGame::instance()->setNeedsPaintSync();
        }
        break;

    case 0:
        // paint "high scores" asset
        vid.BG0_drawAsset(Vec2(0,0), ScorePan, ScorePan.frames - 1);
        vid.BG0_drawAsset(Vec2(1,0), HighScores);
        BG1Helper bg1(getStateMachine().getCube());

        for (int i = arraysize(SavedData::sHighScores) - 1;
             i >= 0;
             --i)
        {
            if (SavedData::sHighScores[i] == 0)
            {
                break;
            }
            char string[17];
            sprintf(string, "%d", SavedData::sHighScores[i]);
            paintScoreNumbers(bg1,
                              Vec2(SCORE_RHS_X,SCORE_RHS_Y + (arraysize(SavedData::sHighScores) - 1 - i) * 2),
                              string);
        }
        bg1.Flush(); // TODO only flush if mask has changed recently
        WordGame::instance()->setNeedsPaintSync();

        break;
    }
}
