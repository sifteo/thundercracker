/*
 * Sifteo SDK Example.
 */

#include <sifteo.h>
#include "game.h"


GameCube::GameCube(CubeID cube)
    : hilighter(vid),
      portal_e(TOP),
      portal_w(LEFT),
      portal_f(BOTTOM),
      portal_a(RIGHT),
      portalTicker(50),
      numMarkers(0)
{
    vid.attach(cube);
}
      
void GameCube::init()
{
    vid.initMode(BG0_SPR_BG1);
    hilighter.init();
    vid.bg0.image(vec(0,0), Playfield);
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
        p.draw(vid);
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

    vid.bg0.image(vec(3 + x*2, 3 + y*2), Markers, id);

    numMarkers++;
}

Float2 GameCube::velocityFromTilt()
{
    return vid.physicalAccel().xy() * 0.1f;
}

bool GameCube::reportMatches(unsigned bits)
{
    switch (bits) {
    case 0xF:                   placeMarker(1); return true;
    case 0xF ^ (1 << LEFT):     placeMarker(2); return true;
    case 0xF ^ (1 << TOP):      placeMarker(3); return true;
    case 0xF ^ (1 << BOTTOM):   placeMarker(4); return true;
    case 0xF ^ (1 << RIGHT):    placeMarker(5); return true;
    }
    return false;
}

CubeHilighter::CubeHilighter(VideoBuffer &vid)
    : vid(vid), ticker(6), counter(0), pos(vec(-1,-1))
{}

void CubeHilighter::init()
{
    vid.bg1.setMask(BG1Mask::filled(vec(0,0), Brackets.tileSize()));
    vid.bg1.image(vec(0,0), Brackets);
}

void CubeHilighter::animate(float timeStep)
{
    if (counter > 0)
        counter -= ticker.tick(timeStep);
}

void CubeHilighter::draw()
{
    Int2 v;
    
    if (counter <= 0 || (counter & 1) == 0) {
        // Hidden / flashing
        v = Brackets.pixelSize();
    
    } else {
        // Active
        v = -pos;
    }
    
    v += Brackets.pixelExtent();
    vid.bg1.setPanning(v);
}

bool CubeHilighter::doHilight(Int2 requestedPos)
{
    bool posMatch = pos == requestedPos;

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
 