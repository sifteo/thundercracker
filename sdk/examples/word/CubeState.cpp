#include <sifteo.h>
#include <math.h>
#include "CubeState.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "TileTransparencyLookup.h"
#include "PartialAnimationData.h"
#include "config.h"

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
    if (teethImageIndex == ImageIndex_Teeth && reverseAnim)
    {
        // no blip when reversing teeth anim (use the same anim, but
        // diff transparency data, to save room)
        teethImageIndex = ImageIndex_Teeth_NoBlip;
    }
    const AssetImage* teethImages[NumImageIndexes] =
    {
        &TeethLoopWordTop,     // ImageIndex_Connected,
        &TeethNewWord2,      // ImageIndex_ConnectedWord,
        &TeethLoopWordLeftTop, // ImageIndex_ConnectedLeft,
        &TeethNewWord2Left,  // ImageIndex_ConnectedLeftWord,
        &TeethLoopWordRightTop,// ImageIndex_ConnectedRight,
        &TeethNewWord2Right, // ImageIndex_ConnectedRightWord,
        &TeethLoopNeighboredTop,// ImageIndex_Neighbored,
        &Teeth,             // ImageIndex_Teeth,
        &Teeth,             // ImageIndex_Teeth_NoBlip,
    };

    const AssetImage* teethNumberImages[] =
    {
        &TeethNewWord3,
        &TeethNewWord4,
        &TeethNewWord5,
    };

    const Vec2 TEETH_NUM_POS(4, 6);
    STATIC_ASSERT(arraysize(teethImages) == NumImageIndexes);
    ASSERT(teethImageIndex >= 0);
    ASSERT(teethImageIndex < (ImageIndex)arraysize(teethImages));
    const AssetImage* teeth = teethImages[teethImageIndex];
    const AssetImage* teethNumber = 0;

    unsigned teethNumberIndex = GameStateMachine::getNewWordLength() / MAX_LETTERS_PER_CUBE;
    if (teethNumberIndex > 2)
    {
        teethNumberIndex -= 3;
        switch (teethImageIndex)
        {
        case ImageIndex_ConnectedWord:
        case ImageIndex_ConnectedLeftWord:
        case ImageIndex_ConnectedRightWord:
            teethNumber = teethNumberImages[MIN(((unsigned)arraysize(teethNumberImages) - 1), teethNumberIndex)];
            break;

        default:
            break;
        }
    }
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
        frame = (unsigned) (animTime * teeth->frames);
        frame = MIN(frame, teeth->frames - 1);

        //DEBUG_LOG(("shuffle: [c: %d] anim: %d, frame: %d, time: %f\n", getStateMachine().getCube().id(), teethImageIndex, frame, GameStateMachine::getTime()));

    }
    else if (reverseAnim)
    {
        frame = teeth->frames - 1;
    }

    BG1Helper bg1(mStateMachine->getCube());
    for (unsigned int i = 0; i < 16; ++i) // rows
    {
        for (unsigned j=0; j < 16; ++j) // columns
        {

            Vec2 texCoord(j, i);
            switch (teethImageIndex)
            {
            case ImageIndex_Connected:
                if (i >= 14)
                {
                    teeth = &TeethLoopWordBottom;
                    texCoord.y = i - 14;
                }
                break;

            case ImageIndex_Neighbored:
                if (i >= 14)
                {
                    teeth = &TeethLoopNeighboredBottom;
                    texCoord.y = i - 14;
                }
                break;

            case ImageIndex_ConnectedLeft:
                if (i >= 14)
                {
                    teeth = &TeethLoopWordLeftBottom;
                    texCoord.y = i - 14;
                }
                else if (i >= 2)
                {
                    teeth = &TeethLoopWordLeftLeft;
                    texCoord.x = j % teeth->width;
                    texCoord.y = i - 2;
                }
                break;

            case ImageIndex_ConnectedRight:
                if (i >= 14)
                {
                    teeth = &TeethLoopWordRightBottom;
                    texCoord.y = i - 14;
                }
                else if (i >= 2)
                {
                    teeth = &TeethLoopWordRightRight;
                    texCoord.x = j % teeth->width;
                    texCoord.y = i - 2;
                }
                break;

            default:
                break;
            }

            switch (getTransparencyType(teethImageIndex, frame, j, i))
            {
            case TransparencyType_None:
                // paint this opaque tile
                // paint BG0
                if (teethNumber &&
                    frame >= 2 && frame - 2 < teethNumber->frames &&
                    j >= ((unsigned) TEETH_NUM_POS.x) &&
                    j < teethNumber->width + ((unsigned) TEETH_NUM_POS.x) &&
                    i >= ((unsigned) TEETH_NUM_POS.y) &&
                    i < teethNumber->height + ((unsigned) TEETH_NUM_POS.y))
                {
                    vid.BG0_drawPartialAsset(Vec2(j, i),
                                             Vec2(j - TEETH_NUM_POS.x, i - TEETH_NUM_POS.y),
                                             Vec2(1, 1),
                                             *teethNumber,
                                             frame - 2);
                }
                else
                {
                    vid.BG0_drawPartialAsset(Vec2(j, i), texCoord, Vec2(1, 1), *teeth, frame);
                }
                break;

            case TransparencyType_Some:
                if (teethNumber &&
                    frame >= 2 && frame - 2 < teethNumber->frames &&
                    j >= ((unsigned) TEETH_NUM_POS.x) &&
                    j < teethNumber->width + ((unsigned) TEETH_NUM_POS.x) &&
                    i >= ((unsigned) TEETH_NUM_POS.y) &&
                    i < teethNumber->height + ((unsigned) TEETH_NUM_POS.y))
                {
                    bg1.DrawPartialAsset(Vec2(j, i),
                                         Vec2(j - TEETH_NUM_POS.x, i - TEETH_NUM_POS.y),
                                         Vec2(1, 1),
                                         *teethNumber,
                                         frame - 2);
                }
                else
                {
                    bg1.DrawPartialAsset(Vec2(j, i), texCoord, Vec2(1, 1), *teeth, frame);
                }
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

        // 1, 2, 3, 10, 20, 30
        float animLength[6] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

        // 1, 2, 3, 10, 20, 30
        const AssetImage* lowDigitAnim[6] =
            {&TeethClockPulse1, &TeethClockPulse2, &TeethClockPulse3,
             &TeethClockPulse0, &TeethClockPulse0, &TeethClockPulse0};

        const AssetImage* highDigitAnim[6] =
            {0, 0, 0,
             &TeethClockPulse1, &TeethClockPulse2, &TeethClockPulse3};

        frame = 0;
        float animTime =
                1.f - fmodf(GameStateMachine::getSecondsLeftFloat(), 1.f);
        if (animIndex >= 0 && animTime < animLength[animIndex])
        {
            // normalize
            animTime /= animLength[animIndex];
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
    paintSprites = false;
    vid.BG0_drawAsset(Vec2(0,0), ScreenOff);
    const char *str = getStateMachine().getLetters();
    switch (strlen(str))
    {
    default:
        {
            unsigned frame = *str - (int)'A';

            if (frame < font.frames)
            {
                vid.BG0_drawAsset(Vec2(1,3), font, frame);


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
                      1.2f, 2.8f, 1.1f, .28f, .55f, 2.4f
                    };

                    const static float BlinkDelayMin[NumEyePersonalities] =
                    {
                      0.8f, 1.2f, 0.9f, .22f, .45f, 1.6f
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
                      2.2f, 2.8f, 1.1f, .28f, .55f, 2.4f
                    };

                    const static float DirDelayMin[NumEyePersonalities] =
                    {
                      1.8f, 1.2f, 0.9f, .22f, .45f, 1.6f
                    };

                    enum LetterStateSpriteID
                    {
                        LetterStateSpriteID_LeftEye,
                        LetterStateSpriteID_RightEye,
                        LetterStateSpriteID_Zzz,

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
                    ASSERT(ed.ly > 8); // eyes should not be in top or bottom rows, there seems to be a bug with HW sprite positioning
                    ASSERT(ed.ry > 8);
                    ASSERT(ed.ly < 120);
                    ASSERT(ed.ry < 120);

                    EyePersonality personality =
                            (EyePersonality)((getStateMachine().getCube().id() - CUBE_ID_BASE) % NumEyePersonalities);
                    float sleepIdleTime = 10.f + BlinkDelayMax[personality];
                    if (mEyeBlinkDelay <= 0.f && mEyeState != EyeState_Closed)
                    {
                        // blinking closed trumps tilt and direction change timer
                        mEyeState = EyeState_Closed;
                        mEyeBlinkDelay =
                           WordGame::rand(BlinkHoldMin[personality],
                                          BlinkHoldMax[personality]);
                        if (getStateMachine().getIdleTime() > sleepIdleTime)
                        {
                            mAsleep = true;
                            mEyeBlinkDelay = 99999.f;//WordGame::rand(3.f, 10.f);
                        }
                    }
                    // else blink delay > 0 || eye state == closed
                    else if ((mEyeBlinkDelay <= 0.f ||
                              (mAsleep && getStateMachine().getIdleTime() < sleepIdleTime)) ||
                             mEyeState != EyeState_Closed)
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
                            mAsleep = false;
                            vid.hideSprite(LetterStateSpriteID_Zzz);
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

                        if (mAsleep)
                        {
                            unsigned zzzFrame = (unsigned)floorf(getStateMachine().getTime() / 0.5f);
                            vid.setSpriteImage(LetterStateSpriteID_Zzz, LetterZzZ.index + (zzzFrame % LetterZzZ.frames) * LetterZzZ.width * LetterZzZ.height);
                            vid.resizeSprite(LetterStateSpriteID_Zzz, LetterZzZ.width * 8, LetterZzZ.height * 8);
                            vid.moveSprite(LetterStateSpriteID_Zzz,
                                           MIN(128 - LetterZzZ.width * 8, ed.rx + EyeRight.width * 8 + 2),
                                           MAX(0, 24 - (2 + LetterZzZ.height * 8)));
                        }
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

    case 2:
        {
            unsigned frame = str[0] - (int)'A';

            if (frame < font.frames)
            {
                vid.BG0_drawAsset(Vec2(0,4), font, frame);
            }
            frame = str[1] - (int)'A';

            if (frame < font.frames)
            {
                vid.BG0_drawAsset(Vec2(8,4), font, frame);
            }

        }
      break;

    //case 3:
        // TODO
      //  break;
    }

}

void CubeState::paintScoreNumbers(BG1Helper &bg1, const Vec2& position_RHS, const char* string)
{
    Vec2 position(position_RHS);
    const AssetImage& font = FontSmall;
    unsigned len = strlen(string);

    const unsigned MAX_SCORE_STRLEN = 7;
    const char* MAX_SCORE_STR = "9999999";

    if (len > MAX_SCORE_STRLEN)
    {
        string = MAX_SCORE_STR;
        len = MAX_SCORE_STRLEN;
    }
    position.x -= len - 1;

    for (; *string; ++string)
    {
        unsigned index = *string - '0';
        ASSERT(index < font.frames);
        position.x++;
        bg1.DrawAsset(position, font, index);
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


