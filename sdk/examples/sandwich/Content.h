#pragma once
#include <sifteo.h>

using namespace Sifteo;

#define ITEM_NONE           0
#define ITEM_BASIC_KEY      1
#define ITEM_BREAD          2
#define ITEM_TOMATO         3
#define ITEM_LETTUCE        4
#define ITEM_HAM            5
#define ITEM_SKELETON_KEY   6
#define ITEM_TYPE_COUNT     7

#define TRIGGER_UNDEFINED   0
#define TRIGGER_GATEWAY     1
#define TRIGGER_ITEM        2
#define TRIGGER_NPC         3
#define TRIGGER_TYPE_COUNT  4

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
    uint32_t lineCount;
    const DialogTextData* lines;
};

struct TriggerData {
    uint8_t questBegin; // 0xff means start-at-beginning
    uint8_t questEnd; // 0xff means means never-stop
    uint8_t flagId; // could be 1-32 is local, 33-64 is global
    uint8_t room;
};

struct DoorData {
    uint8_t roomId;
    uint8_t flagId; // doors are associated with the default quest for the map
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
    uint16_t dialog;
    uint8_t x;
    uint8_t y;
};

struct AnimatedTileData {
    uint8_t tileId;
    uint8_t frameCount;
};

struct RoomData {
    uint8_t collisionMaskRows[8];
    uint8_t tiles[64];
    const uint8_t* overlay; // format: alternate 0bXXXXYYYY, tileId, 0bXXXXYYYY, tileId, etc
    uint8_t centerx;
    uint8_t centery;
};

struct MapData {
    const AssetImage* tileset;
    const AssetImage* overlay;
    const AssetImage* blankImage;
    const RoomData* rooms;
    const uint8_t* xportals; // bit array of portals between rooms (x,y) and (x+1,y)
    const uint8_t* yportals; // bit array of portals between rooms (x,y) and (x,y+1)
    const ItemData* items; 
    const GatewayData* gates;
    const NpcData* npcs;
    const DoorData* doors;
    const AnimatedTileData* animatedTiles;
    uint8_t itemCount;
    uint8_t gateCount;
    uint8_t npcCount;
    uint8_t doorQuestId; // 0xff if doors are all global (probably not intentional)
    uint8_t doorCount;
    uint8_t animatedTileCount;
    uint8_t width : 4;
    uint8_t height : 4;
};

extern const unsigned gMapCount;
extern const unsigned gQuestCount;
extern const unsigned gDialogCount;
extern const MapData gMapData[];
extern const QuestData gQuestData[];
extern const DialogData gDialogData[];
