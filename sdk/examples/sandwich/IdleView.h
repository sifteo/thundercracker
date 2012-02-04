#pragma once
#include "View.h"

class IdleView : public View {
private:
    unsigned mStartFrame;
    unsigned mCount;


public:
	void Init();
	void Update();
	void OnInventoryChanged();
	void DrawInventorySprites();
	void DrawBackground();
};