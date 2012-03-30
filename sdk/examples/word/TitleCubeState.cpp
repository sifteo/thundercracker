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

const float SHAKE_DELAY = 3.5f;

unsigned TitleCubeState::onEvent(unsigned eventID, const EventData& data)
{
    switch (eventID)
    {
    // TODO debug: case EventID_Paint:
    case EventID_EnterState:
        mShakeDelay = 0.f;
        mPanning = -16.f;// * ((getStateMachine().getCube().id() & 1) ? -1.f : 1.f);
        paint();
        if (eventID == EventID_EnterState)
        {
            WordGame::instance()->setNeedsPaintSync();
        }
        break;

    case EventID_Paint:
        paint();
        break;

    case EventID_GameStateChanged:
        switch (data.mGameStateChanged.mNewStateIndex)
        {
        case GameStateIndex_StartOfRoundScored:
            return CubeStateIndex_StartOfRoundScored;

        case GameStateIndex_PlayScored:
            return CubeStateIndex_NotWordScored;
        }
        break;
    }
    return getStateMachine().getCurrentStateIndex();
}

unsigned TitleCubeState::update(float dt, float stateTime)
{
    mShakeDelay -= dt;
    if (mShakeDelay <= 0.f)
    {
        mShakeDelay = SHAKE_DELAY;
    }

    _SYSAccelState accelState;
    _SYS_getAccel(getStateMachine().getCube().id(), &accelState);
    _SYSShakeState shakeState;
    _SYS_getShake(getStateMachine().getCube().id(), &shakeState);
    /* TODO remove _SYSTiltState tiltState;
    _SYS_getTilt(getStateMachine().getCube().id(), &tiltState);
*/
    if (shakeState == NOT_SHAKING && abs<int>(accelState.x) > 10)
    {
        mShakeDelay = 0.f;
        mPanning += dt * -5.f * accelState.x; // FIXME treat as accel, not velocity set
    }
    /*if (mPanning != 0.f)
    {
        DEBUG_LOG(("panning %f\n", mPanning));
    }*/
    //mPanning = fmodf(mPanning, 128.f);
    if (fabs(mPanning) > 86.f)
    {
        GameStateMachine::sOnEvent(EventID_Start, EventData());
        return CubeStateIndex_StartOfRoundScored;
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
    /* TODO remove
    {
        vid.BG0_drawAsset(Vec2(0,0), Test);
    }

    return;
*/
    const float ANIM_START_DELAY = 2.f;

    switch (getStateMachine().getCube().id() - CUBE_ID_BASE)
    {
    //default:
#if (0)
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
                bg1.Flush();
            }
        }
        break;
#endif

    default:
    case 1:
        vid.BG0_drawAsset(Vec2(0, 0), StartBG);
        vid.setSpriteImage(0, StartPrompt);
        vid.resizeSprite(0, StartPrompt.width * 8, StartPrompt.height * 8);
        {
            float shakeOffset = 0.f;
            /* misleads player to shake
            if (mShakeDelay < 0.5f)
            {
                const float SHAKE = 4.f;
                shakeOffset = SHAKE/2.f - WordGame::random.uniform(0.f, SHAKE);
            }
            */
            vid.moveSprite(0, Vec2(39 - shakeOffset, 74));
            vid.BG1_setPanning(Vec2((int)mPanning + shakeOffset, 0));
        }
        {            
            BG1Helper bg1(getStateMachine().getCube());
            bg1.DrawAsset(Vec2(0, 0), StartLid);
            bg1.Flush();
        }
        break;

        // TODO high scores
#if BLAH
    default:
        paintBorder(vid, ImageIndex_Teeth);
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
