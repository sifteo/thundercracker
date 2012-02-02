#include "StateMachine.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

namespace TotalsGame
{
	StateMachine::StateMachine()
	{
		numStateNodes = 0;
		numStateTransitions = 0;
		pendingTransition = NULL;
		currentState = NULL;
	}

	StateMachine &StateMachine::State(const char *name, IStateController *controller)
	{
		assert(numStateNodes < MAX_STATE_NODES);

		stateNodes[numStateNodes].controller = controller;
		stateNodes[numStateNodes].isController = true;
		stateNodes[numStateNodes].name = name;

		numStateNodes++;

		return *this;
	}

	StateMachine &StateMachine::State(const char *name, StateTransitionCallback transition)
	{
		assert(numStateNodes < MAX_STATE_NODES);

		stateNodes[numStateNodes].callback = transition;
		stateNodes[numStateNodes].isController = false;
		stateNodes[numStateNodes].name = name;

		numStateNodes++;

		return *this;
	}

	StateMachine &StateMachine::Transition(const char *from, const char *name, const char *to)
	{
		assert(numStateNodes < MAX_STATE_TRANSITIONS);

		stateTransitions[numStateTransitions].from = from;
		stateTransitions[numStateTransitions].name = name;
		stateTransitions[numStateTransitions].to = to;

		numStateTransitions++;

		return *this;
	}

	void StateMachine::Tick(float dt)
	{
		assert(currentState);

		if(currentState->isController)
		{
			currentState->controller->OnTick(dt);
		}
		else
		{
			pendingTransition = currentState->callback();
		}

		if(pendingTransition)
		{
			for(int i = 0; i < numStateTransitions; i++)
			{
				if(!strcmp(stateTransitions[i].from, currentState->name)
					&& !strcmp(stateTransitions[i].name, pendingTransition))
				{
					SetState(stateTransitions[i].to);
					break;
				}
			}

			pendingTransition = NULL;
		}
	}

	void StateMachine::Paint(bool dirty)
	{
		if(currentState->isController)
		{
			currentState->controller->OnPaint(dirty);
		}
	}

	void StateMachine::QueueTransition(const char *name)
	{
		pendingTransition = name;
	}

	void StateMachine::SetState(const char *name)
	{
		if(currentState && currentState->isController)
		{
			currentState->controller->OnDispose();
		}

		for(int i = 0; i < numStateNodes; i++)
		{
			if(!strcmp(stateNodes[i].name, name))
			{
				currentState = stateNodes + i;
				if(currentState->isController)
				{
					currentState->controller->OnSetup();
				}
				return;
			}
		}
		currentState = NULL;
	}

}

