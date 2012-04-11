#pragma once
#include <sifteo.h>

class Viewport;

class View {
public:
	Viewport* Parent() const { 
		// this works because the union of all subviews is the
		// first parameter in Viewport.  Otherwise, we'd
		// need to implemented some offsetof() magic here :P
		return (Viewport*)this; 
	}
};

