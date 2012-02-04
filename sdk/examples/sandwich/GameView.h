#pragma once
#include "RoomView.h"
#include "IdleView.h"
#include "InventoryView.h"

#define VIEW_IDLE		0
#define VIEW_ROOM		1
#define VIEW_INVENTORY	2

class GameView {
private:
	union { // modal views must be first in order to allow casting
		IdleView idle;
		RoomView room;
		InventoryView inventory;
	} mSubview;
	uint8_t mStatus;
  struct {
    unsigned subview : 2;
    unsigned prevTouch : 1;
  } mFlags;

public:

	Cube* GetCube() const;
	Cube::ID GetCubeID() const;
	bool Touched() const;
	inline unsigned Subview() const { return mFlags.subview ; }
	inline bool IsShowingRoom() const { return mFlags.subview == VIEW_ROOM; }
	inline IdleView* GetIdleView() { ASSERT(mFlags.subview == VIEW_IDLE); return &(mSubview.idle); }
	inline RoomView* GetRoomView() { ASSERT(mFlags.subview == VIEW_ROOM); return &(mSubview.room); }
	inline InventoryView* GetInventoryView() { ASSERT(mFlags.subview == VIEW_INVENTORY); return &(mSubview.inventory); }

	void Init();
	void Restore();
	void Update();
  
  	void HideSprites();

	bool ShowLocation(Vec2 location);
	bool HideLocation();
	void RefreshInventory();

	Cube::Side VirtualTiltDirection() const;
	GameView* VirtualNeighborAt(Cube::Side side) const;

};
