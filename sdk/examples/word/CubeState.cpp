#include "CubeState.h"
#include "assets.gen.h"

void CubeState::setCube(Cube* cube)
{
    ASSERT(cube > 0);
    mCube = cube;

    VidMode_BG0 vid(cube->vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), Background);

    // Clear BG1/SPR before switching modes
    _SYS_vbuf_fill(&cube->vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&cube->vbuf.sys, _SYS_VA_BG1_TILES/2,
                   cube->vbuf.indexWord(Font.index), 32);
    _SYS_vbuf_fill(&cube->vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);

    // Allocate 16x2 tiles on BG1 for text at the bottom of the screen
    _SYS_vbuf_fill(&cube->vbuf.sys, _SYS_VA_BG1_BITMAP/2 + 14, 0xFFFF, 2);

    // Switch modes
    _SYS_vbuf_pokeb(&cube->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);

}
