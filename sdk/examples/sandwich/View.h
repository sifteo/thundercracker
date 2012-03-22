#pragma once
#include <sifteo.h>

class ViewSlot;

typedef Sifteo::VidMode_BG0_SPR_BG1 ViewMode;

class View {
public:
	inline ViewSlot* Parent() const { 
		// this works because the union of all subviews is the
		// first parameter in ViewSlot.  Otherwise, we'd
		// need to implemented some offsetof() magic here :P
		return (ViewSlot*)this; 
	}
};