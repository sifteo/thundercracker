#pragma once
#include <sifteo.h>

using namespace Sifteo;

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

#define STORAGE_INVENTORY   0
#define STORAGE_EQUIPMENT   1
#define STORAGE_TYPE_COUNT  2

#define ITEM_TRIGGER_NONE   0
#define ITEM_TRIGGER_KEY    1
#define ITEM_TRIGGER_BOOT   2
#define ITEM_TRIGGER_BOMB   3

#define EVENT_NONE                          0
#define EVENT_ADVANCE_QUEST_AND_REFRESH     1
#define EVENT_ADVANCE_QUEST_AND_TELEPORT    2
#define EVENT_REMOTE_TRIGGER                3
#define EVENT_OPEN_DOOR                     4

struct QuestData {
    uint8_t mapId;
    uint8_t roomId;
};

struct ItemTypeData {
    const char* description;
    uint8_t storageType : 1;
    uint8_t triggerType : 7; 
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
    uint8_t eventType;
};

struct DoorData {
    uint8_t roomId;
    uint8_t flagId; // doors are associated with the default quest for the map
};

struct ItemData {
    TriggerData trigger;
    uint8_t itemId;
};

struct GatewayData {
    TriggerData trigger;
    uint8_t targetMap;
    uint8_t targetGate;
    uint8_t x;
    uint8_t y;
};

struct SokoblockData {
    uint16_t x;
    uint16_t y;
    uint8_t asset;
};

struct NpcData {
    TriggerData trigger;
    uint16_t dialog : 15;
    uint16_t optional : 1;
    uint8_t x;
    uint8_t y;
};

struct TrapdoorData {
    uint8_t roomId;
    uint8_t respawnRoomId;
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

typedef uint8_t TileSetID;

// todo - microoptimize bits
// todo - replace pointers with <32bit offsets-from-known-locations?
// todo - separate tilesets from maps?  (e.g. animated tiles, lava tiles)
struct MapData {
    const char* name;

    // stir pointers
    const AssetImage* tileset;
    const AssetImage* overlay;

    // tile buffers
    const RoomData* rooms;
    const uint8_t* rle_overlay; // overlay layer w/ empty-tiles RLE-encoded (tileId, tileId, 0xff, emptyCount, tileId, ...)
    const uint8_t* xportals; // bit array of portals between rooms (x,y) and (x+1,y)
    const uint8_t* yportals; // bit array of portals between rooms (x,y) and (x,y+1)

    // triggers
    const ItemData* items; 
    const GatewayData* gates;
    const NpcData* npcs;
    const TrapdoorData* trapdoors;

    // other placeable entities
    const DoorData* doors;
    const AnimatedTileData* animatedTiles;
    const TileSetID* lavaTiles;
    const DiagonalSubdivisionData* diagonalSubdivisions;
    const BridgeSubdivisionData* bridgeSubdivisions;
    const SokoblockData* sokoblocks;

    // trigger counts
    uint8_t itemCount;
    uint8_t gateCount;
    uint8_t npcCount;
    uint8_t trapdoorCount;

    // other counts
    uint8_t doorQuestId; // 0xff if doors are all global (probably not intentional)
    uint8_t doorCount;
    uint8_t animatedTileCount;
    uint8_t diagonalSubdivisionCount;
    uint8_t bridgeSubdivisionCount;
    uint8_t sokoblockCount;
    uint8_t ambientType; // 0 - None

    // size
    uint8_t width : 4;
    uint8_t height : 4;
};

extern const unsigned gMapCount;
extern const unsigned gQuestCount;
extern const unsigned gDialogCount;
extern const MapData gMapData[];
extern const QuestData gQuestData[];
extern const DialogData gDialogData[];
extern const ItemTypeData gItemTypeData[];

extern const PinnedAssetImage* gSokoblockAssets[];
