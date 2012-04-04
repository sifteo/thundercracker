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

static Platform platform1(0, Vec2(64, 64));
static Platform platform2(1, Vec2(32, 96));
static Thing michelangelo(2, Vec2(32, 0));

const int NUM_THINGS = 3;
static Thing *things[NUM_THINGS] = {&platform1, &platform2, &michelangelo};

static TimeStep timeStep;

void init()
{
    Metadata()
        .title("TMNT: Ninja Slide");
//         .icon(GameIcon);

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

    vid.setSpriteImage(platform1.id, tile_platform01);
    vid.setSpriteImage(platform2.id, tile_platform02);

    timeStep.next();
    timeStep.next();

    while (1) {

        for(int i=0; i < NUM_THINGS; i++) things[i]->think(cube.id());
        float dt = timeStep.delta().seconds();
        for(int i=0; i < NUM_THINGS; i++) things[i]->act(dt);
        for(int i=0; i < NUM_THINGS; i++) things[i]->draw(vid);

        System::paint();
        timeStep.next();
    }
}
