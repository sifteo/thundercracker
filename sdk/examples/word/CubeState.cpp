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
