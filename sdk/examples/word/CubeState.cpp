#include <sifteo.h>
#include <math.h>
#include "CubeState.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "TileTransparencyLookup.h"
#include "PartialAnimationData.h"

void CubeState::setStateMachine(CubeStateMachine& csm)
{
    mStateMachine = &csm;

    Cube& cube = csm.getCube();
    VidMode_BG0_SPR_BG1 vid(cube.vbuf);
    vid.init();
}

CubeStateMachine& CubeState::getStateMachine()
{
    ASSERT(mStateMachine != 0);
    return *mStateMachine;
}

void CubeState::paintTeeth(VidMode_BG0_SPR_BG1& vid,
                           ImageIndex teethImageIndex,
                           bool animate,
                           bool reverseAnim,
                           bool loopAnim,
                           bool paintTime,
                           float animStartTime)
{
    const AssetImage* teethImages[] =
    {
        &TeethLoopWord,     // ImageIndex_Connected,
        &TeethNewWord,      // ImageIndex_ConnectedWord,
        &TeethLoopWordLeft, // ImageIndex_ConnectedLeft,
        &TeethNewWordLeft,  // ImageIndex_ConnectedLeftWord,
        &TeethLoopWordRight,// ImageIndex_ConnectedRight,
        &TeethNewWordRight, // ImageIndex_ConnectedRightWord,
        &TeethLoopConnected,// ImageIndex_Neighbored,
        &Teeth,             // ImageIndex_Teeth,
    };

    const AssetImage* teethCenterImages[] =
    {
        &TeethNewWord2,
        &TeethNewWord3,
        &TeethNewWord4,
        &TeethNewWord5,
        &TeethNewWord6,
    };

    const AssetImage* teethLeftImages[] =
    {
        &TeethNewWord2Left,
        &TeethNewWord3Left,
        &TeethNewWord4Left,
        &TeethNewWord5Left,
        &TeethNewWord6Left,
    };

    const AssetImage* teethRightImages[] =
    {
        &TeethNewWord2Right,
        &TeethNewWord3Right,
        &TeethNewWord4Right,
        &TeethNewWord5Right,
        &TeethNewWord6Right,
    };

    STATIC_ASSERT(arraysize(teethImages) == NumImageIndexes);
    ASSERT(teethImageIndex >= 0);
    ASSERT(teethImageIndex < (ImageIndex)arraysize(teethImages));
    const AssetImage* pteeth = teethImages[teethImageIndex];

    switch (teethImageIndex)
    {
    case ImageIndex_ConnectedWord:
        pteeth = teethCenterImages[MIN(((unsigned)arraysize(teethCenterImages) - 1), ((unsigned)GameStateMachine::getNewWordLength() - 2))];
        break;

    case ImageIndex_ConnectedLeftWord:
        pteeth = teethCenterImages[MIN(((unsigned)arraysize(teethLeftImages) - 1), ((unsigned)GameStateMachine::getNewWordLength() - 2))];
        break;

    case ImageIndex_ConnectedRightWord:
        pteeth = teethCenterImages[MIN(((unsigned)arraysize(teethRightImages) - 1), ((unsigned)GameStateMachine::getNewWordLength() - 2))];
        break;

    default:
        break;
    }

    const AssetImage& teeth = *pteeth;

    unsigned frame = 0;
    unsigned secondsLeft = GameStateMachine::getSecondsLeft();

    if (animate)
    {
        float animTime =  (getStateMachine().getTime() - animStartTime) / TEETH_ANIM_LENGTH;
        if (loopAnim)
        {
            animTime = fmodf(animTime, 1.0f);
        }
        else
        {
            animTime = MIN(animTime, 1.f);
        }

        if (reverseAnim)
        {
            animTime = 1.f - animTime;
        }
        frame = (unsigned) (animTime * teeth.frames);
        frame = MIN(frame, teeth.frames - 1);

        // HACK skip blip if reversing anim (chomp)
        if (reverseAnim && (frame == 4 || frame == 5))
        {
            frame = 3;
        }
        //DEBUG_LOG(("shuffle: [c: %d] anim: %d, frame: %d, time: %f\n", getStateMachine().getCube().id(), teethImageIndex, frame, GameStateMachine::getTime()));

    }
    else if (reverseAnim)
    {
        frame = teeth.frames - 1;
    }

    BG1Helper bg1(mStateMachine->getCube());
    // scan frame for non-transparent rows and adjust partial draw window
    const uint16_t* tiles = &teeth.tiles[frame * teeth.width * teeth.height];
    for (int i = teeth.height - 1; i >= 0; --i) // rows
    {
        for (unsigned j=0; j < teeth.width; ++j) // columns
        {
            switch (getTransparencyType(teethImageIndex, frame, j, i))
            {
            case TransparencyType_None:
                // paint this opaque tile
                // paint BG0
                vid.BG0_drawPartialAsset(Vec2(j, i), Vec2(j, i), Vec2(1, 1), teeth, frame);
                break;

            case TransparencyType_Some:
                bg1.DrawPartialAsset(Vec2(j, i), Vec2(j, i), Vec2(1, 1), teeth, frame);
                break;

            default:
                ASSERT(getTransparencyType(teethImageIndex, frame, j, i) == TransparencyType_All);
                break;
            }
        }
    }

    if (paintTime)
    {
        int animIndex = -1;
        /* TODO
        switch (secondsLeft)
        {
        case 30:
        case 20:
        case 10:
            animIndex = secondsLeft/10 + 2;
            break;

        case 3:
        case 2:
        case 1:
            animIndex = secondsLeft - 1;
            break;
        }

        */

        // 1, 2, 3, 10, 20, 30
        float animLength[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

        // 1, 2, 3, 10, 20, 30
        const AssetImage* lowDigitAnim[6] =
            {&TeethClock1, &TeethClock2, &TeethClock3,
             &TeethClock30_0, &TeethClock30_0, &TeethClock30_0};

        const AssetImage* highDigitAnim[6] =
            {0, 0, 0,
             &TeethClock10_1, &TeethClock20_2, &TeethClock30_3};

        frame = 0;
        if (animIndex >= 0)
        {
            float animTime =  getStateMachine().getTime() / animLength[animIndex];
            animTime = MIN(animTime, 1.f);
            frame = (unsigned) (animTime * lowDigitAnim[animIndex]->frames);
            frame = MIN(frame, lowDigitAnim[animIndex]->frames - 1);

            if (highDigitAnim[animIndex] > 0)
            {
                bg1.DrawAsset(Vec2(((3 - 2 + 0) * 4 + 1), 14),
                              *highDigitAnim[animIndex],
                              frame);
            }
            bg1.DrawAsset(Vec2(((3 - 2 + 1) * 4 + 1), 14),
                          *lowDigitAnim[animIndex],
                          frame);
        }
        else
        {
            char string[5];
            sprintf(string, "%d", secondsLeft);
            unsigned len = strlen(string);
            for (unsigned i = 0; i < len; ++i)
            {
                frame = string[i] - '0';
                bg1.DrawAsset(Vec2(((3 - strlen(string) + i) * 4 + 1), 14),
                              FontTeeth,
                              frame);
            }
        }
    }

    bg1.Flush(); // TODO only flush if mask has changed recently
    WordGame::instance()->setNeedsPaintSync();
}

void CubeState::paintLetters(VidMode_BG0_SPR_BG1 &vid, const AssetImage &font, bool paintSprites)
{
    const char *str = getStateMachine().getLetters();
    switch (strlen(str))
    {
    default:
        {
            unsigned frame = *str - (int)'A';

            if (frame < font.frames)
            {
                vid.BG0_drawAsset(Vec2(0,0), font, frame);


                if (paintSprites)
                {
                    enum EyePersonality
                    {
                        EyePersonality_Calm = 0,
                        EyePersonality_Wired,
                        EyePersonality_Sleepy,
                        EyePersonality_Blinky,
                        EyePersonality_Shy,
                        EyePersonality_Foo,

                        NumEyePersonalities
                    };

                    const static float BlinkDelayMax[NumEyePersonalities] =
                    {
                      2.2f, 8.8f, 1.1f, .28f, .55f, 4.4f
                    };

                    const static float BlinkDelayMin[NumEyePersonalities] =
                    {
                      1.8f, 7.2f, 0.9f, .22f, .45f, 3.6f
                    };

                    const static float BlinkHoldMax[NumEyePersonalities] =
                    {
                      0.1375f, 0.55f, 0.06875f, 0.0175f, 0.034375f, 0.275f
                    };

                    const static float BlinkHoldMin[NumEyePersonalities] =
                    {
                      0.1125f, 0.45f, .05f, 0.01375f, 0.0275f, 0.225f
                    };

                    const static float DirDelayMax[NumEyePersonalities] =
                    {
                      2.2f, 8.8f, 1.1f, .28f, .55f, 4.4f
                    };

                    const static float DirDelayMin[NumEyePersonalities] =
                    {
                      1.8f, 7.2f, 0.9f, .22f, .45f, 3.6f
                    };

                    enum LetterStateSpriteID
                    {
                        LetterStateSpriteID_LeftEye,
                        LetterStateSpriteID_RightEye,

                        NumLetterStateSpriteIDs
                    };

                    const static EyeState tiltStateToEyeState[3][3] =
                    {
                        // x == 0
                        {EyeState_NW, EyeState_W, EyeState_SW},

                        // x == 1
                        {EyeState_N, EyeState_Center, EyeState_S},

                        // x == 2
                        {EyeState_NE, EyeState_E, EyeState_SE},
                    };

                    unsigned eyeFrame = 0;
                    const EyeData& ed = getEyeData(*str);

                    EyePersonality personality =
                            (EyePersonality)(getStateMachine().getCube().id() % NumEyePersonalities);
                    if (mEyeBlinkDelay <= 0.f && mEyeState != EyeState_Closed)
                    {
                        // blinking closed trumps tilt and direction change timer
                        mEyeState = EyeState_Closed;
                        mEyeBlinkDelay =
                            WordGame::rand(BlinkHoldMin[personality],
                                           BlinkHoldMax[personality]);
                    }
                    // else blink delay > 0 || eye state == closed
                    else if (mEyeBlinkDelay <= 0.f || mEyeState != EyeState_Closed)
                    {
                        // else if time to open eyes, or eyes open and not time to close eyes
                        // tilt trumps direction change timer
                        _SYSTiltState state;
                        _SYS_getTilt(getStateMachine().getCube().id(), &state);
                        EyeState newEyeState = tiltStateToEyeState[state.x][state.y];
                        if (newEyeState == EyeState_Center)
                        {
                            // not tilted
                            if (mEyeDirChangeDelay <= 0.f || mEyeBlinkDelay <= 0.f)
                            {
                                // time to open eyes or change dir
                                newEyeState = (EyeState)WordGame::rand(NumEyeStates);
                                mEyeDirChangeDelay =
                                        WordGame::rand(DirDelayMin[personality],
                                                       DirDelayMax[personality]);
                            }
                            else
                            {
                                // waiting on timers
                                newEyeState = mEyeState;
                            }
                        }
                        else
                        {
                            // tilted, correct newEyeState already determined
                        }

                        if (mEyeState == EyeState_Closed)
                        {
                            // opening eyes
                            mEyeBlinkDelay =
                                WordGame::rand(BlinkDelayMin[personality],
                                               BlinkDelayMax[personality]);
                        }
                        mEyeState = newEyeState;
                    }

                    switch (mEyeState)
                    {
                    case EyeState_Closed:
                        vid.setSpriteImage(LetterStateSpriteID_LeftEye, EyeLeftBlink.index);
                        vid.resizeSprite(LetterStateSpriteID_LeftEye, EyeLeft.width * 8, EyeLeft.height * 8);
                        vid.moveSprite(LetterStateSpriteID_LeftEye, ed.lx, ed.ly);

                        vid.setSpriteImage(LetterStateSpriteID_RightEye, EyeRightBlink.index);
                        vid.resizeSprite(LetterStateSpriteID_RightEye, EyeRight.width * 8, EyeRight.height * 8);
                        vid.moveSprite(LetterStateSpriteID_RightEye, ed.rx, ed.ry);
                        break;

                    case EyeState_Center:
                        WordGame::hideSprites(vid);
                        break;

                    default:
                        {
                            unsigned eyeFrame = MAX(0, NumEyeStates - mEyeState); // asset frames are backwards
                            vid.setSpriteImage(LetterStateSpriteID_LeftEye, EyeLeft.index + (eyeFrame % EyeLeft.frames) * EyeLeft.width * EyeLeft.height);
                            vid.resizeSprite(LetterStateSpriteID_LeftEye, EyeLeft.width * 8, EyeLeft.height * 8);
                            vid.moveSprite(LetterStateSpriteID_LeftEye, ed.lx, ed.ly);

                            vid.setSpriteImage(LetterStateSpriteID_RightEye, EyeRight.index + (eyeFrame % EyeRight.frames) * EyeRight.width * EyeRight.height);
                            vid.resizeSprite(LetterStateSpriteID_RightEye, EyeRight.width * 8, EyeRight.height * 8);
                            vid.moveSprite(LetterStateSpriteID_RightEye, ed.rx, ed.ry);
                        }
                        break;
                    }

                }
                else
                {
                    WordGame::hideSprites(vid);
                }
            }
        }
        break;

    //case 2:
        // TODO
        /*    while ((c = *str))
    {
        if (c == '\n')
        {
            p.x = point.x;
            p.y += Font.width;
        }
        else
        {
            BG0_text(p, Font, c);
            p.x += Font.height;
        }
        str++;
    }
*/
      //  break;

    //case 3:
        // TODO
      //  break;
    }

}

void CubeState::paintScoreNumbers(VidMode_BG0_SPR_BG1 &vid, const Vec2& position, const char* string)
{
    const AssetImage& font = FontSmall;
    for (; *string; ++string)
    {
        unsigned index;
        switch (*string)
        {
        default:
            index = *string - '0';
            break;

        case '+':
            index = 10;
            break;

        case ' ':
            index = 11;
            break;
        }

        vid.BG0_drawAsset(position, font, index);
    }
}

unsigned CubeState::update(float dt, float stateTime)
{
    mEyeDirChangeDelay -= dt;
    mEyeDirChangeDelay = MAX(0.f, mEyeDirChangeDelay);
    mEyeBlinkDelay -= dt;
    mEyeBlinkDelay = MAX(0.f, mEyeBlinkDelay);

    return getStateMachine().getCurrentStateIndex();
}


