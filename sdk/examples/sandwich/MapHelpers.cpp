#include "MapHelpers.h"


Side ComputeGateSide(const GatewayData* gate) {
	int dx = gate->x - 64;
	int dy = gate->y - 64;
	int adx = abs(dx);
	int ady = abs(dy);
	if (adx > ady) {
		return dx > 0 ? RIGHT : LEFT;
	} else if (ady > adx) {
		return dy > 0 ? BOTTOM : TOP;
	}	
	return SIDE_UNDEFINED;
}