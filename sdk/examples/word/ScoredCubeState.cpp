#include "ScoredCubeState.h"
#include "EventID.h"
#include "assets.gen.h"


ScoredCubeState::ScoredCubeState()
{
}

unsigned ScoredCubeState::onEvent(unsigned eventID)
{
    switch (eventID)
    {
    case EventID_Paint:
        // TODO paint
        break;

    case EventID_EnterState:
        VidMode_BG0 vid(mCube->vbuf);
        vid.init();
        vid.BG0_drawAsset(Vec2(0,0), Background);

        // Clear BG1/SPR before switching modes
        _SYS_vbuf_fill(&mCube->vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
        _SYS_vbuf_fill(&mCube->vbuf.sys, _SYS_VA_BG1_TILES/2,
                       mCube->vbuf.indexWord(Font.index), 32);
        _SYS_vbuf_fill(&mCube->vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);

        // Allocate 16x2 tiles on BG1 for text at the bottom of the screen
        _SYS_vbuf_fill(&mCube->vbuf.sys, _SYS_VA_BG1_BITMAP/2 + 14, 0xFFFF, 2);

        // Switch modes
        _SYS_vbuf_pokeb(&mCube->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
        break;

    }
    return 0;
}
