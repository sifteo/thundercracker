#pragma once
#include "Common.h"
#include "View.h"
#include "Content.h"
#include "DrawingHelpers.h"

class MinimapView : public View {
private:
	int8_t mCanvasOffsetX;
	int8_t mCanvasOffsetY;

public:
	void Init();
	void Restore();
	void Update(float dt);
private:
	unsigned ComputeTileId(int lx, int ly);
};