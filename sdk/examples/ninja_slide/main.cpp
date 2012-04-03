/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

enum SpriteIndices {
    turtle,
    platform
};

static Int2 platform_pos;

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
     
    vid.setSpriteImage(turtle, Michelangelo);
    vid.setSpriteImage(platform, Platform);
    platform_pos.x = 64;
    platform_pos.y = 64;
    

    int x = 0;
    const Int2 center = { (128 - 16)/2, (128 - 16)/2 };

    while (1) {
        x = (x+1) % 128;
        int y = 60;
        platform_pos.y = (platform_pos.y + 1) % 128;
        vid.moveSprite(turtle, Vec2(x,y));
        vid.moveSprite(platform, platform_pos);

        System::paint();
    }
}
