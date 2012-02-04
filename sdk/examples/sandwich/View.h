#pragma once

class GameView;

class View {
public:
	inline GameView* Parent() const { 
		// this works because the union of all subviews is the
		// first parameter in GameView.  Otherwise, we'd
		// need to implemented some offsetof() magic here :P
		return (GameView*)this; 
	}
};