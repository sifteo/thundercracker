#pragma once
#include "RoomView.h"
#include "IdleView.h"
#include "InventoryView.h"
#include "MinimapView.h"

#define VIEW_NONE		0
#define VIEW_IDLE		1
#define VIEW_ROOM		2
#define VIEW_INVENTORY	3
#define VIEW_MINIMAP	4
#define VIEW_TYPE_COUNT	5

class ViewSlot {
private:
	union { // modal views must be first in order to allow casting
		IdleView idle;
		RoomView room;
		InventoryView inventory;
		MinimapView minimap;
	} mView;
  	struct {
    	unsigned view 		: 3;
    	unsigned prevTouch	: 1;
  	} mFlags;

public:

	Cube* GetCube() const;
	Cube::ID GetCubeID() const;
	ViewMode Graphics() const { ASSERT(mFlags.view); return ViewMode(GetCube()->vbuf); }
	BG1Helper Overlay() const { ASSERT(mFlags.view); return BG1Helper(*GetCube()); }
	bool Touched() const; // cube->touching && !prevTouch
	bool Active() const { return mFlags.view; }
	inline unsigned ViewType() const { return mFlags.view ; }
	inline bool IsShowingRoom() const { return mFlags.view == VIEW_ROOM; }
	inline IdleView* GetIdleView() { ASSERT(mFlags.view == VIEW_IDLE); return &(mView.idle); }
	inline RoomView* GetRoomView() { ASSERT(mFlags.view == VIEW_ROOM); return &(mView.room); }
	inline InventoryView* GetInventoryView() { ASSERT(mFlags.view == VIEW_INVENTORY); return &(mView.inventory); }
	inline MinimapView* GetMinimapView() { ASSERT(mFlags.view == VIEW_MINIMAP); return &(mView.minimap); }

	void Init();
	void Restore(bool doFlush=true);
	void Update(float dt);
  
  	void HideSprites();

	bool ShowLocation(Vec2 location, bool doFlush=true);
	bool HideLocation(bool doFlush=true);

	void RefreshInventory(bool doFlush=true);

	Cube::Side VirtualTiltDirection() const;
	ViewSlot* VirtualNeighborAt(Cube::Side side) const;

private:
	void SetView(unsigned viewId, bool doFlush, unsigned rig=0);
	ViewSlot* FindIdleView();
};

extern ViewSlot *pInventory;
extern ViewSlot *pMinimap;
