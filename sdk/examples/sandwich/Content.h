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

#define SUBDIV_NONE         0
#define SUBDIV_DIAG_POS     1
#define SUBDIV_DIAG_NEG     2
#define SUBDIV_BRDG_HOR     3
#define SUBDIV_BRDG_VER     4


struct QuestData {
    uint8_t mapId;
    uint8_t roomId;
};

struct DialogTextData {
    const AssetImage* detail;
    const char* line;
};

struct DialogData {
    const AssetImage* npc;
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

struct RoomData { // expect to support about 1,000 rooms max (10 maps * 81 rooms rounded up)
    uint8_t collisionMaskRows[8];
    uint8_t tiles[64];
    uint8_t centerX : 4;
    uint8_t centerY : 4;
};

struct DiagonalSubdivisionData {
    uint8_t positiveSlope : 1;
    uint8_t roomId : 7;
    uint8_t altCenterX : 4;
    uint8_t altCenterY : 4;
};

struct BridgeSubdivisionData {
    uint8_t isHorizontal : 1;
    uint8_t roomId : 7;
    uint8_t altCenterX : 4;
    uint8_t altCenterY : 4;
};

// todo - microoptimize bits
// todo - replace pointers with <32bit offsets-from-known-locations?
struct MapData {
    const AssetImage* tileset;
    const AssetImage* overlay;
    const RoomData* rooms;
    const uint8_t* rle_overlay; // overlay layer w/ empty-tiles RLE-encoded (tileId, tileId, 0xff, emptyCount, tileId, ...)
    const uint8_t* xportals; // bit array of portals between rooms (x,y) and (x+1,y)
    const uint8_t* yportals; // bit array of portals between rooms (x,y) and (x,y+1)
    const ItemData* items; 
    const GatewayData* gates;
    const NpcData* npcs;
    const DoorData* doors;
    const AnimatedTileData* animatedTiles;
    const DiagonalSubdivisionData* diagonalSubdivisions;
    const BridgeSubdivisionData* bridgeSubdivisions;
    uint8_t itemCount;
    uint8_t gateCount;
    uint8_t npcCount;
    uint8_t doorQuestId; // 0xff if doors are all global (probably not intentional)
    uint8_t doorCount;
    uint8_t animatedTileCount;
    uint8_t diagonalSubdivisionCount;
    uint8_t bridgeSubdivisionCount;
    uint8_t width;
    uint8_t height;
    uint8_t ambientType; // 0 - None
};

extern const unsigned gMapCount;
extern const unsigned gQuestCount;
extern const unsigned gDialogCount;
extern const MapData gMapData[];
extern const QuestData gQuestData[];
extern const DialogData gDialogData[];
