#pragma once
#include "View.h"

class IdleView : public View {
private:
    unsigned mStartFrame;
    unsigned mCount;


public:
	void Init();
	void Restore();
	inline void Update(float dt) {}
	//void OnInventoryChanged();

private:
	//void DrawInventorySprites();
};