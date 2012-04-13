#pragma once
#include "Common.h"
#include "Content.h"
#include "View.h"

void WaitForSeconds(float dt);


void DrawRoom(Viewport& vp, int roomId);
void DrawRoomOverlay(Viewport& vp, unsigned tid, const uint8_t *pRle);
void DrawOffsetMap(Viewport& vp, Int2 pos);

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
