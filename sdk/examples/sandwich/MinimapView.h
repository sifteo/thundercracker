#pragma once
#include "Common.h"
#include "View.h"
#include "Content.h"
#include "DrawingHelpers.h"

class MinimapView : public View {
private:
	Byte2 mCanvasOffset;

public:
	void Init();
	void Restore();
	void Update(float dt);
private:
	unsigned ComputeTileId(int lx, int ly);
};