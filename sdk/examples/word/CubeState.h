#ifndef CUBESTATE_H
#define CUBESTATE_H

#include <sifteo.h>
#include "State.h"

enum CubeStateIndex
{
    CubeStateIndex_Title,
    CubeStateIndex_TitleExit,
    CubeStateIndex_NotWordScored,
    CubeStateIndex_NewWordScored,
    CubeStateIndex_OldWordScored,
    CubeStateIndex_EndOfRoundScored,

    CubeStateIndex_NumStates
};

class CubeStateMachine;

const float TEETH_ANIM_LENGTH = 1.0f;

class CubeState : public State
{
public:
    CubeState() : mStateMachine(0) { }
    void setStateMachine(CubeStateMachine& csm);
    CubeStateMachine& getStateMachine();

protected:
    void paintTeeth(VidMode_BG0_SPR_BG1& vid,
                    const AssetImage& teeth,
                    bool animate=false,
                    bool reverseAnim=false,
                    bool loopAnim=false,
                    bool paintTime=false);
    void paintLetters(VidMode_BG0_SPR_BG1 &vid, const AssetImage &font);
    void paintScoreNumbers(VidMode_BG0_SPR_BG1 &vid, const Vec2& position, const char* string);

private:
    CubeStateMachine* mStateMachine;
};

#endif // CUBESTATE_H
