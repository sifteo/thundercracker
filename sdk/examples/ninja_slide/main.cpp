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
    static World world;
   
    int id=0;
    static Turtle michelangelo(world, id++, Vec2(32, 32));
    static LPlatform platform1(world, id++, Vec2(64, 64));
    static Platform platform2(world, id++, Vec2(32, 96));
    static Thing static_platform(world, id++, Vec2(32, 64));

    static Thing *things[] = {&michelangelo, &static_platform, &platform1, &platform2};
    static Thing *platforms[] = {&static_platform, &platform1, &platform2};


    loadAssets();

    VidMode_BG0_SPR_BG1 vid(cube.vbuf);
    vid.init();

    // fill background
    for(UInt2 pos = Vec2(0, 0); pos.x < VidMode::LCD_width / VidMode::TILE; pos.x += tile_bckgrnd01.width){
        for(pos.y = 0; pos.y < VidMode::LCD_height / VidMode::TILE; pos.y += tile_bckgrnd01.height){
            vid.BG0_drawAsset(pos, tile_bckgrnd01);
        }
    }
    
    static_platform.setSpriteImage(vid, tile_platform_static01);
    platform1.setSpriteImage(vid, tile_platform01);
    platform2.setSpriteImage(vid, tile_platform02);
    michelangelo.setSpriteImage(vid, Michelangelo);

    while (1) {
        world.mainLoopReset();
        float dt = timeStep.delta().seconds();

        if ( 1 || world.platformsMustStop){
            michelangelo.think(cube.id());
            michelangelo.act(dt);
        } else {
            for(int i=0; i < arraysize(platforms); i++) platforms[i]->think(cube.id());
            for(int i=0; i < arraysize(platforms); i++) platforms[i]->act(dt);
            collisions(platforms, arraysize(platforms));
        }

        for(int i=0; i < arraysize(things); i++) things[i]->draw(vid);

        System::paint();
        timeStep.next();
    }
}

