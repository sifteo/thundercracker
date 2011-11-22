#include "GameStateMachine.h"

GameStateMachine::GameStateMachine(int startStateIndex) :
    StateMachine(startStateIndex)
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
