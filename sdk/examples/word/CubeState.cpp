#include "CubeState.h"
#include "assets.gen.h"

void CubeState::setCube(Cube& cube)
{
    mCube = &cube;


    // Clear BG1/SPR before switching modes
    _SYS_vbuf_fill(&mCube->vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&mCube->vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);

    // Switch modes
    _SYS_vbuf_pokeb(&mCube->vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
}


