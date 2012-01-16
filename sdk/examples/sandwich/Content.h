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

struct QuestData {
    uint8_t mapId;
};

struct DialogData {
    uint32_t todo;
};

struct ItemData {
    uint32_t room;
    uint32_t itemId;
};

struct GatewayData {
    uint32_t todo;
};

struct NpcData {
    uint32_t todo;
};

struct RoomData {
    uint8_t collisionMaskRows[8];
    uint8_t tiles[64];
    const uint8_t* overlay; // format: alternate 0bXXXXYYYY, tileId, 0bXXXXYYYY, tileId, etc
    uint32_t centerx : 4;
    uint32_t centery : 4;
    uint32_t reserved : 24;
};

struct MapData {
    const AssetImage* tileset;
    const AssetImage* overlay;
    const AssetImage* blankImage;
    const RoomData* rooms;
    const uint8_t* xportals; // vertical portals between rooms (x,y) and (x+1,y)
    const uint8_t* yportals; // horizontal portals between rooms (x,y) and (x,y+1)
    uint8_t width;
    uint8_t height;
    const ItemData* items; // null if empty, itemId=0-termindated
    uint16_t reserved;
};

extern const MapData gMapData[];
extern const QuestData gQuestData[];
extern const DialogData gDialogData[];
