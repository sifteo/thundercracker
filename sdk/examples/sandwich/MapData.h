#pragma once
#include <sifteo.h>

using namespace Sifteo;

#define PORTAL_OPEN	0
#define PORTAL_WALL	1
#define PORTAL_DOOR 2

#define ITEM_NONE           0
#define ITEM_BASIC_KEY      1
#define ITEM_BREAD          2
#define ITEM_TOMATO         3
#define ITEM_LETTUCE        4
#define ITEM_HAM            5
#define ITEM_SKELETON_KEY   6

#define TRIGGER_TYPE_PASSIVE    0
#define TRIGGER_TYPE_ACTIVE     1

typedef void (*trigger_func)(int);

struct TriggerData {
    int room;
    trigger_func callback;
};

struct ItemData {
    int room;
    uint32_t itemId;
};

struct RoomData {
    uint16_t centerPosition; // format: 0b0000000000XXXYYY (any use for those high bits?)
    uint8_t collisionMaskRows[8];
    uint8_t tiles[64];
    uint8_t torch0; // 0bXXXXYYYY
    uint8_t torch1; // 0bXXXXYYYY
    uint8_t* overlay; // format: alternative 0bXXXXYYYY, tileId, 0bXXXXYYYY, tileId, etc

    inline Vec2 LocalCenter() const { return Vec2((centerPosition >> 3) & 0x7, centerPosition & 0x7); }
    inline Vec2 TorchTile0() const { return Vec2(torch0 >> 4, torch0 & 0xf); }
    inline Vec2 TorchTile1() const { return Vec2(torch1 >> 4, torch1 & 0xf); }
};

struct MapData {
    const AssetImage* tileset;
    const AssetImage* overlay;
    const AssetImage* blankImage;
    RoomData* rooms;
    uint8_t* xportals; // vertical portals between rooms (x,y) and (x+1,y)
    uint8_t* yportals; // horizontal portals between rooms (x,y) and (x,y+1)
    TriggerData* triggers; // null if empty, callback=0-terminated
    ItemData* items; // null if empty, itemId=0-termindated
    uint8_t width;
    uint8_t height;
    
    // Convenience Methods

    inline uint8_t GetPortalX(int x, int y) const {
        // note that the pitch here is one greater than the width because there's
        // an extra wall on the right side of the last tile in each row
        ASSERT(0 <= x && x <= width);
        ASSERT(0 <= y && y < height);
        return xportals[y * (width+1) + x];
    }

    inline uint8_t GetPortalY(int x, int y) const {
        // Like GetPortalX except we're in column-major order
        ASSERT(0 <= x && x < width);
        ASSERT(0 <= y && y <= height);
        return yportals[x * (height+1) + y];
    }
    
    inline void SetPortalX(int x, int y, uint8_t pid) {
        ASSERT(0 <= x && x <= width);
        ASSERT(0 <= y && y < height);
        xportals[y * (width+1) + x] = pid;
    }

    inline void SetPortalY(int x, int y, uint8_t pid) {
        ASSERT(0 <= x && x < width);
        ASSERT(0 <= y && y <= height);
        yportals[x * (height+1) + y] = pid;
    }

    inline RoomData* GetRoomData(uint8_t roomId) const {
        ASSERT(roomId < width * height);
        return rooms + roomId;
    }

    inline RoomData* GetRoomData(Vec2 location) const {
        return GetRoomData(GetRoomId(location));
    }

    inline uint8_t GetTileId(Vec2 location, Vec2 tile) const {
        // this value indexes into tileset.frames
        ASSERT(0 <= location.x && location.x < width);
        ASSERT(0 <= location.y && location.y < height);
        ASSERT(0 <= tile.x && tile.x < 8);
        ASSERT(0 <= tile.y && tile.y < 8);
        return rooms[location.y * width + location.x].tiles[tile.y * 8 + tile.x];
    }

    inline void SetTileId(Vec2 location, Vec2 tile, uint8_t tileId) {
        ASSERT(0 <= location.x && location.x < width);
        ASSERT(0 <= location.y && location.y < height);
        ASSERT(0 <= tile.x && tile.x < 8);
        ASSERT(0 <= tile.y && tile.y < 8);
        rooms[location.y * width + location.x].tiles[tile.y * 8 + tile.x] = tileId;
    }

    inline bool IsTileOpen(Vec2 location, Vec2 tile) {
        ASSERT(0 <= location.x && location.x < width);
        ASSERT(0 <= location.y && location.y < height);
        ASSERT(0 <= tile.x && tile.x < 8);
        ASSERT(0 <= tile.y && tile.y < 8);
        return ( rooms[location.y * width + location.x].collisionMaskRows[tile.y] & (1<<tile.x) ) == 0;
    }

    inline uint8_t GetRoomId(Vec2 location) const {
        ASSERT(0 <= location.x && location.x < width);
        ASSERT(0 <= location.y && location.y < height);
        return location.x + location.y * width;
    }

    inline Vec2 GetLocation(uint8_t roomId) const {
        ASSERT(roomId < width * height);
        return Vec2(
            roomId % width,
            roomId / width
        );
    }

    inline TriggerData* FindTriggerData(uint8_t roomId) {
        for(TriggerData* p = triggers; p && p->room >= 0; ++p) {
            if (p->room == roomId) {
                return p;
            }
        }
        return 0;
    }

    inline ItemData* FindItemData(uint8_t roomId) {
        for(ItemData* p = items; p && p->room >= 0; ++p) {
            if (p->room == roomId) {
                return p;
            }
        }
        return 0;
    }
};
