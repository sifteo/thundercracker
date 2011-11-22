#ifndef GAMESTATEMACHINE_H
#define GAMESTATEMACHINE_H

#include "StateMachine.h"

class GameStateMachine : public StateMachine
{
public:
    GameStateMachine();

    virtual void OnEvent(int eventID);

protected:
    virtual State* GetState(int index) const;
    virtual int GetNumStates() const { return 0; }

};

#endif // GAMESTATEMACHINE_H
