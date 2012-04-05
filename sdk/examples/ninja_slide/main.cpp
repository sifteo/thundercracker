/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
#include "rect.h"
#include "thing.h"
#include "platform.h"
#include "turtle.h"

using namespace Sifteo;

static Cube cube(0);

static LPlatform platform1(0, Vec2(64, 64));
static Platform platform2(1, Vec2(32, 96));
static Turtle michelangelo(2, Vec2(32, 32));

const int NUM_THINGS = 3;
static Thing *things[NUM_THINGS] = {&platform1, &platform2, &michelangelo};

void loadAssets()
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

void collisions(Thing **things, int num_things){
    for (int i = 0; i < num_things; i++){
        for (int j = i+1; j < num_things; j++){
            if (things[i]->isTouching(*things[j])){
                things[i]->collided(things[j]);
                things[j]->collided(things[i]);
            }
        }
    }
}

void main()
{
    static TimeStep timeStep;

    loadAssets();

    VidMode_BG0_SPR_BG1 vid(cube.vbuf);
    vid.init();

    // fill background
    for(UInt2 pos = Vec2(0, 0); pos.x < VidMode::LCD_width / VidMode::TILE; pos.x += tile_bckgrnd01.width){
        for(pos.y = 0; pos.y < VidMode::LCD_height / VidMode::TILE; pos.y += tile_bckgrnd01.height){
            vid.BG0_drawAsset(pos, tile_bckgrnd01);
        }
    }
    
    michelangelo.setSpriteImage(vid, Michelangelo);

    platform1.setSpriteImage(vid, tile_platform01);
    platform2.setSpriteImage(vid, tile_platform02);

    while (1) {

        for(int i=0; i < NUM_THINGS; i++) things[i]->think(cube.id());
        float dt = timeStep.delta().seconds();
        for(int i=0; i < NUM_THINGS; i++) things[i]->act(dt);
        collisions(things, NUM_THINGS);
        for(int i=0; i < NUM_THINGS; i++) things[i]->draw(vid);

        System::paint();
        timeStep.next();
    }
}

