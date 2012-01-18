#pragma once
#include <sifteo.h>

using namespace Sifteo;

#define PORTAL_OPEN	        0
#define PORTAL_WALL	        1
#define PORTAL_DOOR         2

#define ITEM_NONE           0
#define ITEM_BASIC_KEY      1
#define ITEM_BREAD          2
#define ITEM_TOMATO         3
#define ITEM_LETTUCE        4
#define ITEM_HAM            5
#define ITEM_SKELETON_KEY   6

#define TRIGGER_UNDEFINED   -1
#define TRIGGER_GATEWAY     0
#define TRIGGER_ITEM        1
#define TRIGGER_NPC         2

struct QuestData {
    uint8_t mapId;
    uint8_t roomId;
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
    uint8_t questBegin; // 0xff means start-at-beginning
    uint8_t questEnd; // 0xff means means never-stop
    uint8_t flagId; // could be 1-32 is local, 33-64 is global
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
    uint32_t itemCount;
    uint32_t gateCount;
    uint32_t npcCount;
    uint16_t width;
    uint16_t height;
};

extern const unsigned gMapCount;
extern const unsigned gQuestCount;
extern const unsigned gDialogCount;
extern const MapData gMapData[];
extern const QuestData gQuestData[];
extern const DialogData gDialogData[];
