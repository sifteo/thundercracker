#include <sifteo.h>
#include <math.h>
#include "CubeState.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "assets.gen.h"
#include "WordGame.h"
#include "TileTransparencyLookup.h"

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
                           bool paintTime)
{
    const AssetImage* teethImages[NumImageIndexes] =
    {
      &Teeth, &TeethLoopConnected, &TeethLoopWord, &TeethNewWord
    };
    ASSERT(teethImageIndex >= 0);
    ASSERT(teethImageIndex < arraysize(teethImages));
    const AssetImage& teeth = *teethImages[teethImageIndex];
    unsigned frame = 0;
    unsigned secondsLeft = GameStateMachine::getSecondsLeft();

    if (animate)
    {
        float animTime =  getStateMachine().getTime() / TEETH_ANIM_LENGTH;
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
    bg1.Flush();
    WordGame::instance()->setNeedsPaintSync();
}

void CubeState::paintLetters(VidMode_BG0_SPR_BG1 &vid, const AssetImage &font)
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



