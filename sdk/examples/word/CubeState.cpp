#include <sifteo.h>
#include "CubeState.h"
#include "CubeStateMachine.h"
#include "GameStateMachine.h"
#include "assets.gen.h"

void CubeState::setStateMachine(CubeStateMachine& csm)
{
    mStateMachine = &csm;

    Cube& cube = csm.getCube();
    // Clear BG1/SPR before switching modes
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);

    // Switch modes
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
}


CubeStateMachine& CubeState::getStateMachine()
{
    ASSERT(mStateMachine != 0);
    return *mStateMachine;
}

void CubeState::paintTeeth(VidMode_BG0& vid, bool animate, bool reverseAnim, bool paintTime)
{
    unsigned frame = 0;

    if (animate)
    {
        // TODO deal with animStartTime
        float animTime =  getStateMachine().getTime() / TEETH_ANIM_LENGTH;
        animTime = MIN(animTime, 1.f);
        if (reverseAnim)
        {
            animTime = 1.f - animTime;
        }
        frame = (unsigned) (animTime * Teeth.frames);
        frame = MIN(frame, Teeth.frames - 1);
//        frame = frame % Teeth.frames;
    }
    else if (reverseAnim)
    {
        frame = Teeth.frames - 1;
    }
    BG1Helper bg1(mStateMachine->getCube());
    // scan frame for non-transparent rows and adjust partial draw window
    const uint16_t* tiles = &Teeth.tiles[frame * Teeth.width * Teeth.height];
    unsigned rowsPainted = 0;
    const unsigned MAX_BG1_ROWS = 9;
    for (int i=Teeth.height-1; i >= 0; --i) // rows
    {
        uint16_t firstIndex = tiles[i * Teeth.width];
        for (unsigned j=0; j < Teeth.width; ++j) // columns
        {
            if (tiles[j + i * Teeth.width] != firstIndex)
            {
                // paint this opaque row
                if (rowsPainted >= MAX_BG1_ROWS)
                {
                    // TODO paint BG0
                    vid.BG0_drawPartialAsset(Vec2(0, i), Vec2(0, i), Vec2(16, 1), Teeth, frame);
                }
                else
                {
                    bg1.DrawPartialAsset(Vec2(0, i), Vec2(0, i), Vec2(16, 1), Teeth, frame);
                    ++rowsPainted;
                }
                break;
            }
        }
    }

    if (paintTime)
    {
        char string[5];
        sprintf(string, "%d", GameStateMachine::getSecondsLeft());
        unsigned len = strlen(string);
        for (unsigned i = 0; i < len; ++i)
        {
            bg1.DrawAsset(Vec2(((3 - strlen(string) + i) * 4 + 1), 14),
                          FontSmall,
                          string[i] - '0');
        }
    }
    bg1.Flush();
}

void CubeState::paintLetters(VidMode_BG0 &vid, const AssetImage &font)
{
    Vec2 point(0, 0);
    //char c;
    const char *str = getStateMachine().getLetters();
    switch (strlen(str))
    {
    default:
        {
            unsigned index = 0;//*str - (int)'A';
            if (index < font.frames)
            {
                vid.BG0_drawAsset(point, font, index);
            }
        }
        break;

    case 2:
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
        break;

    case 3:
        // TODO
        break;
    }

}

void CubeState::paintNumbers(VidMode_BG0 &vid, const Vec2& position, const char* string)
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

