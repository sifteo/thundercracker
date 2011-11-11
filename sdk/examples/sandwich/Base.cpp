#include "Base.h"

Cube::Side InferDirection(Vec2 u) {
	if (u.x > 0) {
		return SIDE_RIGHT;
	} else if (u.x < 0) {
		return SIDE_LEFT;
	} else if (u.y < 0) {
		return SIDE_TOP;
	} else {
		return SIDE_BOTTOM;
	}
}

