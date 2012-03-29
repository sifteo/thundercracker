#pragma once
#include "Common.h"
#include "Content.h"
#include "View.h"

void WaitForSeconds(float dt);


void DrawRoom(ViewMode* gfx, const MapData* pMap, int roomId);
void DrawRoomOverlay(BG1Helper* ovrly, const MapData* pMap, unsigned tid, const uint8_t *pRle);
void DrawOffsetMap(ViewMode* gfx, const MapData* pMap, Int2 pos);
bool DrawOffsetMapFromTo(ViewMode* gfx, const MapData* pMap, Int2 from, Int2 to);

struct ButterflyFriend {
    
    uint8_t active : 1;
    uint8_t dir : 3;
    uint8_t frame : 4;
    UByte2 pos;

    void Randomize();
    void Update();

};

#define HOVER_COUNT 32
extern const int8_t kHoverTable[HOVER_COUNT];
