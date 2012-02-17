#include "TitleCubeState.h"
#include <sifteo.h>
#include "EventID.h"
#include "EventData.h"
#include "assets.gen.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "SavedData.h"
#include "WordGame.h"
#include "config.h"


const float SMOKE_ANIM_LENGTH = 2.f;

unsigned TitleCubeState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        mAnimDelay = 0.f;
        // fall through
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

unsigned TitleCubeState::update(float dt, float stateTime)
{
    if (mAnimDelay > 0.f)
    {
        mAnimDelay -= dt;
        mAnimDelay = MAX(0.f, mAnimDelay);
        if (mAnimDelay <= 0.f)
        {
            mAnimStart = true;
        }
    }
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
    VidMode_BG0_SPR_BG1 vid(c.vbuf);
    vid.init();

    const float ANIM_START_DELAY = 2.f;

    switch (getStateMachine().getCube().id() - CUBE_ID_BASE)
    {
    //default:
    case 999:
        vid.BG0_drawAsset(Vec2(0,0), Title);
        if (mAnimDelay <= 0.f)
        {
            if (mAnimStart)
            {
                mAnimStart = false;
                mAnimStartTime = getStateMachine().getTime();
            }

            if (getStateMachine().getTime() - mAnimStartTime >= SMOKE_ANIM_LENGTH)
            {
                if (mFirstAnimDelay)
                {
                    mFirstAnimDelay = false;
                    mAnimStart = true;
                    mAnimDelay = 0.f;
                }
                else
                {
                    mAnimDelay = WordGame::random.uniform(2.f, 4.f);
                }
            }
            else
            {
                const AssetImage& anim = TitleSmoke;
                float animTime =
                        fmodf(getStateMachine().getTime() - mAnimStartTime, SMOKE_ANIM_LENGTH) / SMOKE_ANIM_LENGTH;
                animTime = MIN(animTime, 1.f);
                unsigned frame = (unsigned) (animTime * anim.frames);
                frame = MIN(frame, anim.frames - 1);

                BG1Helper bg1(getStateMachine().getCube());
                bg1.DrawAsset(Vec2(8, 0), anim, frame);
                bg1.Flush(); // TODO only flush if mask has changed recently
                WordGame::instance()->setNeedsPaintSync();
            }
        }
        break;

    default:
    case 1:
        vid.BG0_drawAsset(Vec2(0, 0), Teeth);
        if (getStateMachine().getTime() > SMOKE_ANIM_LENGTH)
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

        // TODO high scores
#if BLAH
    default:
        paintTeeth(vid, ImageIndex_Teeth);
        /* TODO load/save
        paintScoreNumbers(vid, Vec2(3,4), FontSmall, "High Scores");

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
            paintScoreNumbers(vid, Vec2(5,4 + (arraysize(SavedData::sHighScores) - i) * 2),
                         FontSmall,
                         string);
        }
        */
        break;
#endif
    }
}
