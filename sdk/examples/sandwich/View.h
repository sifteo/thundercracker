#pragma once

class ViewSlot;

class View {
public:
	inline ViewSlot* Parent() const { 
		// this works because the union of all subviews is the
		// first parameter in ViewSlot.  Otherwise, we'd
		// need to implemented some offsetof() magic here :P
		return (ViewSlot*)this; 
	}
};