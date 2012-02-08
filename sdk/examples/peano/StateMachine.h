#pragma once

namespace TotalsGame
{
	class IStateController
	{
	public:
		virtual void OnSetup();// = 0;
		virtual void OnDispose();// = 0;
		virtual float OnTick(float dt);// = 0;
		virtual void OnPaint(bool dirty);// = 0;
	};

	typedef const char *(*StateTransitionCallback)(void);

	class StateMachine
	{
	public:
		StateMachine();
		
		StateMachine &State(const char *name, IStateController *controller);
		StateMachine &State(const char *name, StateTransitionCallback transition);
		StateMachine &Transition(const char *from, const char *name, const char *to);

		void Tick(float dt);
		void Paint(bool dirty);

		void QueueTransition(const char *name);
		void SetState(const char *name);

	private:
		struct StateNode
		{
			union
			{
				IStateController *controller;
				StateTransitionCallback callback;
			};
			bool isController;
			const char *name;
		};

		struct StateTransition
		{
			const char *from, *name, *to;
		};

		static const int MAX_STATE_NODES = 16;
		static const int MAX_STATE_TRANSITIONS = 16;

		StateNode stateNodes[MAX_STATE_NODES];
		int numStateNodes;

		StateTransition stateTransitions[MAX_STATE_TRANSITIONS];
		int numStateTransitions;

		const char *pendingTransition;

		StateNode *currentState;

		float nextFrameDelay;

	};


}
