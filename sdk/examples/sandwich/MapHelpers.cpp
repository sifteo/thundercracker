#include "MapHelpers.h"


Cube::Side ComputeGateSide(const GatewayData* gate) {
	int dx = gate->x - 64;
	int dy = gate->y - 64;
	int adx = abs(dx);
	int ady = abs(dy);
	if (adx > ady) {
		return dx > 0 ? SIDE_RIGHT : SIDE_LEFT;
	} else if (ady > adx) {
		return dy > 0 ? SIDE_BOTTOM : SIDE_TOP;
	}	
	return SIDE_UNDEFINED;
}