/* Copyright <c> 2011 Sifteo, Inc. All rights reserved. */

#include <stdlib.h>
#include <sifteo.h>

#include "game.h"


GameCube::GameCube(int id)
    : cube(id),
      hilighter(cube),
      portal_e(SIDE_TOP),
      portal_w(SIDE_LEFT),
      portal_f(SIDE_BOTTOM),
      portal_a(SIDE_RIGHT),
      portalTicker(50),
      numMarkers(0)
{}
      

void GameCube::init()
{
    VidMode_BG0_SPR_BG1 vid(cube.vbuf);

    vid.init();
    hilighter.init();
    vid.BG0_drawAsset(Vec2(0,0), Playfield);
}

void GameCube::animate(float timeStep)
{
    for (int t = portalTicker.tick(timeStep); t; t--)
        for (int i = 0; i < NUM_SIDES; i++) {
            Portal &p = getPortal(i);        
            p.animate();
        }
    
    hilighter.animate(timeStep);
}

void GameCube::draw()
{
    for (int i = 0; i < NUM_SIDES; i++) {
        Portal &p = getPortal(i);
        p.draw(cube);
    }
    
    hilighter.draw();
}

void GameCube::placeMarker(int id)
{
    const unsigned size = 5;
    
    if (numMarkers >= size * size)
        return;
    
    unsigned x = numMarkers % size;
    unsigned y = numMarkers / size;
    
    VidMode_BG0 vid(cube.vbuf);
    vid.BG0_drawAsset(Vec2(3 + x*2, 3 + y*2), Markers, id);
    
    numMarkers++;
}

Float2 GameCube::velocityFromTilt()
{
    _SYSAccelState accel;
    _SYS_getAccel(cube.id(), &accel);
    
    const float coeff = 0.1f;
    return Float2( accel.x * coeff, accel.y * coeff );
}

bool GameCube::reportMatches(unsigned bits)
{
    switch (bits) {
    case 0xF:                        placeMarker(1); return true;
    case 0xF ^ (1 << SIDE_LEFT):     placeMarker(2); return true;
    case 0xF ^ (1 << SIDE_TOP):      placeMarker(3); return true;
    case 0xF ^ (1 << SIDE_BOTTOM):   placeMarker(4); return true;
    case 0xF ^ (1 << SIDE_RIGHT):    placeMarker(5); return true;
    }
    return false;
}

CubeHilighter::CubeHilighter(Cube &cube)
    : cube(cube), ticker(6), counter(0), pos(-1,-1)
{}

void CubeHilighter::init()
{
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2,
                   ((1 << Brackets.width) - 1), Brackets.height);
    _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                     Brackets.tiles, 0, Brackets.width * Brackets.height);
}

void CubeHilighter::animate(float timeStep)
{
    if (counter > 0)
        counter -= ticker.tick(timeStep);
}

void CubeHilighter::draw()
{
    int8_t x, y;
    
    if (counter <= 0 || (counter & 1) == 0) {
        // Hidden / flashing
        x = -Brackets.width * 8;
        y = -Brackets.height * 8;
    
    } else {
        // Active
        x = pos.x;
        y = pos.y;
    }
    
    x -= Brackets.width * 4;
    y -= Brackets.height * 4;
    
    x = -x;
    y = -y;
    _SYS_vbuf_poke(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_x) / 2,
                   ((uint8_t)x) | ((uint16_t)(uint8_t)y << 8));
}

bool CubeHilighter::doHilight(Vec2 requestedPos)
{
    bool posMatch = pos.x == requestedPos.x && pos.y == requestedPos.y;

    if (counter <= 0) {
        if (posMatch) {
            // Finished
            return true;
        }
    
        // Start something new
        counter = 3;
        pos = requestedPos;
    }

    return false;
}
 