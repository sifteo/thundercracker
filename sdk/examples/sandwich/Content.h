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

struct DialogTextData {
    const AssetImage* detail;
    const char* line;
};

struct DialogData {
    const PinnedAssetImage* npc;
    const DialogTextData* text;
};

struct TriggerData {
    uint8_t questBegin;
    uint8_t questEnd;
    uint8_t flagId; // could be global or local flag
    uint8_t room;
};

struct ItemData {
    TriggerData trigger;
    uint32_t itemId;
};

struct GatewayData {
    TriggerData trigger;
    uint8_t targetMap;
    uint8_t targetGate;
    uint8_t x;
    uint8_t y;
};

struct NpcData {
    TriggerData trigger;
    uint32_t dialog;
};

struct RoomData {
    uint8_t collisionMaskRows[8];
    uint8_t tiles[64];
    const uint8_t* overlay; // format: alternate 0bXXXXYYYY, tileId, 0bXXXXYYYY, tileId, etc
    uint16_t centerx;
    uint16_t centery;
};

struct MapData {
    const AssetImage* tileset;
    const AssetImage* overlay;
    const AssetImage* blankImage;
    const RoomData* rooms;
    const uint8_t* xportals; // vertical portals between rooms (x,y) and (x+1,y)
    const uint8_t* yportals; // horizontal portals between rooms (x,y) and (x,y+1)
    const ItemData* items; 
    const GatewayData* gates;
    const NpcData* npcs;
    uint32_t item_count : 11;
    uint32_t gate_count : 10;
    uint32_t npc_count : 11;
    uint16_t width;
    uint16_t height;
};

extern const MapData gMapData[];
extern const QuestData gQuestData[];
extern const DialogData gDialogData[];
