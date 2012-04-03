/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

class Thing {
public:
    int id;
    Int2 pos;
    Thing(int id, int x, int y);
    void update(VidMode_BG0_SPR_BG1 vid);
};

Thing::Thing(int id0, int x, int y){
    id = id0;
    pos.x = x;
    pos.y = y;
}

void Thing::update(VidMode_BG0_SPR_BG1 vid){
    vid.moveSprite(id, pos);
}


static Thing platform(0, 64, 64);
static Thing michelangelo(1, 32, 0);

void init()
{

    cube.enable();
    cube.loadAssets(GameAssets);

    VidMode_BG0_ROM vid(cube.vbuf);
    vid.init();
    do {
        vid.BG0_progressBar(Vec2(0,7), cube.assetProgress(GameAssets, VidMode_BG0::LCD_width) & ~3, 2); 
        System::paint();
    } while (!cube.assetDone(GameAssets));
}

void main()
{
    init();



    VidMode_BG0_SPR_BG1 vid(cube.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), MyBackground);
    
    vid.setSpriteImage(michelangelo.id, Michelangelo);
    vid.setSpriteImage(platform.id, Platform);

//     _SYSTiltState tilt = _SYS_getTilt(0);
    

    const Int2 center = { (128 - 16)/2, (128 - 16)/2 };

    while (1) {
        michelangelo.pos.x = (michelangelo.pos.x + 1) % 128;
        platform.pos.y = (platform.pos.y + 1) % 128;

        michelangelo.update(vid);
        platform.update(vid);

        System::paint();
    }
}
