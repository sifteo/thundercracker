/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
#include "thing.h"

using namespace Sifteo;

static Cube cube(0);

static class Platform platform(0, Vec2(64, 64));
static Thing michelangelo(1, Vec2(32, 0));

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
    michelangelo.vel = Vec2(5, 10);

    vid.setSpriteImage(platform.id, Platform);

//     const Int2 center = { (128 - 16)/2, (128 - 16)/2 };

    while (1) {

        michelangelo.think();
        platform.think(cube.id());

        michelangelo.act(0.05);
        platform.act(0.05);

        michelangelo.draw(vid);
        platform.draw(vid);

        System::paint();
    }
}
