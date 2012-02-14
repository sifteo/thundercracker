#pragma once
#include "Common.h"
#include "Content.h"
#include "View.h"

void DrawRoom(ViewMode* gfx, const MapData* pMap, int roomId);
void DrawOffsetMap(ViewMode* gfx, const MapData* pMap, Vec2 pos);
bool DrawOffsetMapFromTo(ViewMode* gfx, const MapData* pMap, Vec2 from, Vec2 to);

struct ButterflyFriend {
    
    uint8_t active : 1;
    uint8_t dir : 3;
    uint8_t frame : 4;
    uint8_t x;
    uint8_t y;

    void Randomize();
    void Update();

};
