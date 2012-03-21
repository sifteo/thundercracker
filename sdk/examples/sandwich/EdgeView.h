#pragma once
#include "View.h"

// additions to the regular side enum
#define SIDE_TOP_LEFT		4
#define SIDE_BOTTOM_LEFT	5
#define SIDE_BOTOTM_RIGHT	6
#define SIDE_TOP_RIGHT		7

class EdgeView : View {
private:

public:
	void Init(Int2 location, Cube::Side side);
	void Restore();
	void Update(float dt);	
};