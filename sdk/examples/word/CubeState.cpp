#include <sifteo.h>
#include "CubeState.h"
#include "CubeStateMachine.h"
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

void CubeState::paintTeeth(bool animate)
{
    unsigned frame = 0;

    if (animate)
    {
        frame = (unsigned) (getStateMachine().getTime() * 3.f);
        frame = MIN(frame, Teeth.frames - 1);
    }
    BG1Helper bg1(mStateMachine->getCube());
    // TODO scan frame for non-transparent rows and adjust partial draw window
    bg1.DrawPartialAsset(Vec2(0, frame), Vec2(0, frame), Vec2(16, 2), Teeth, frame);
    bg1.DrawPartialAsset(Vec2(0, 13), Vec2(0, 13), Vec2(16, 3), Teeth, frame);
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
