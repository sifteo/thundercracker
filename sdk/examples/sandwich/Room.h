#pragma once
#include "Base.h"
#include "Content.h"

class Room {
public:
	int itemId;

  int RoomId() const;
  Vec2 Location() const;
  const RoomData* Data() const;

  inline Vec2 Position() const { return 128 * Location(); }
  inline Vec2 LocalCenter() const { return Vec2(Data()->centerx, Data()->centery); }
  inline Vec2 Center() const { return Position() + 16 * LocalCenter(); }

  uint8_t GetPortal(Cube::Side side);
  uint8_t GetTile(Vec2 position);
  void OpenDoor();
};
