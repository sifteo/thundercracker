#include "GameStateMachine.h"

GameStateMachine::GameStateMachine() :
    StateMachine(0)
{
}

State* GameStateMachine::GetState(int index) const
{
    switch (index)
    {
    default:
        return 0;
    }
}

void GameStateMachine::OnEvent(int eventID)
{

}
