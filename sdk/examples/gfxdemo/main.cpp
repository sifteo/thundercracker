/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sifteo.h>

#include "assets.gen.h"
#include "monsters_small.h"

using namespace Sifteo;

static Cube cube(0);

void load()
{
    cube.enable();
    cube.loadAssets(GameAssets);

    VidMode_BG0_ROM vid(cube.vbuf);

    vid.init();

    do {
        vid.BG0_progressBar(Vec2(0,7), cube.assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
        System::paint();
    } while (!cube.assetDone(GameAssets));
}

void solidFrames(uint16_t color, unsigned num)
{
    VidMode_Solid vid(cube.vbuf);

    vid.setWindow(0, 128);
    vid.setColor(color);
    vid.set();

    while (num--)
        System::paint();
}

void whiteFlash()
{
    solidFrames(0xFFFF, 5);
}

void panToEarth()
{
    VidMode_BG0 vid(cube.vbuf);

    vid.setWindow(0, 128);
    vid.clear(Black.tiles[0]);
    vid.BG0_drawAsset(Vec2(0,0), Globe64);
    vid.set();

    // Negative acceleration simulation, with overshoot

    float x = -10.0f;
    float y = 30.0f;
    float vx = 10.0f;
    float vy = -7.0f;

    const float damping = 0.99f;
    const float gain = 0.03f;

    for (unsigned i = 0; i < 160; i++) {
        x += vx;
        y += vy;
        vx -= gain * x;
        vy -= gain * y;
        vx *= damping;
        vy *= damping;

        vid.BG0_setPanning(Vec2( x - (128/2 - 64/2) + 0.5f,
                                 y - (128/2 - 64/2) + 0.5f ));
        System::paint();
    }
}

void earthZoomSlow()
{
    /*
     * Slow rotate/zoom, from a distance
     */

    VidMode_BG2 vid(cube.vbuf);
    const int height = 100;

    solidFrames(0x0000, 3);
    vid.setWindow((128 - height) / 2, height);
    vid.clear(Black.tiles[0]);
    vid.BG2_drawAsset(Vec2(0,0), Globe128);
    vid.BG2_setBorder(0x0000);
    vid.set();

    for (float t = 0; t < 1.0f; t += 0.03f) {
        AffineMatrix m = AffineMatrix::identity();

        m.translate(64, 64);
        m.scale(0.5f + t * 0.4f);
        m.rotate(t * 0.5f);
        m.translate(-64, -64 + (128 - height) / 2);

        vid.BG2_setMatrix(m);
        System::paint();
    }
}

void lbZoom(const Sifteo::AssetImage &asset, int cx, int cy, float z1, float z2)
{
    /*
     * Fast rotate/zoom, close up and letterboxed
     */

    VidMode_BG2 vid(cube.vbuf);
    const int height = 90;

    solidFrames(0x0000, 3);
    vid.setWindow((128 - height) / 2, height);
    vid.clear(Black.tiles[0]);
    vid.BG2_drawAsset(Vec2(0,0), asset);
    vid.BG2_setBorder(0x0000);
    vid.set();

    for (float t = 0; t < 1.0f; t += 0.015f) {
        AffineMatrix m = AffineMatrix::identity();

        m.translate(cx, cy);
        m.scale(z1 + t * z2);
        m.rotate(t * 0.7f - 1.0f);
        m.translate(-cx, -cy);

        vid.BG2_setMatrix(m);
        System::paint();
    }
}

void showMonsters() {
    VidMode vid(cube.vbuf);

    vid.setWindow(0, 128);
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_FB32);

    for (unsigned loops = 0; loops < 2; loops++)
        for (unsigned i = 0; i < arraysize(monsters); i++) {
            const MonsterData *m = monsters[i];

            _SYS_vbuf_write(&cube.vbuf.sys, 0, (const uint16_t *) m, 256);
            _SYS_vbuf_write(&cube.vbuf.sys, 384, 256 + (const uint16_t *) m, 16);

            for (unsigned j = 0; j < 3; j++)
                System::paint();
        }
}

void owlbearRun()
{
    VidMode_BG0 vid(cube.vbuf);

    vid.setWindow(0, 128);
    vid.clear(Black.tiles[0]);
    vid.BG0_setPanning(Vec2(0,0));
    vid.set();

    for (unsigned loops = 0; loops < 20; loops++)
        for (unsigned frame = 0; frame < Owlbear.frames; frame++) {
            vid.BG0_drawAsset(Vec2(0,0), Owlbear, frame);
            System::paint();
        }
}

void brickScroll()
{
    VidMode_BG0 vid(cube.vbuf);

    vid.setWindow(0, 128);
    vid.BG0_drawAsset(Vec2(0,0), Brick144);
    vid.set();
    
    for (unsigned frame = 0; frame < 600; frame++) {

        if (frame == 300) {
            _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_BG1);
            _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2 + 3, 0x0FF0, 9);
            _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2, Cat.tiles, 0, 8*9);
        }

        vid.BG0_setPanning(Vec2( frame * 2.5f + 0.5f, sinf(frame * 0.06f) * 60.0f + 0.5f ));
        System::paint();
    }
}


//get random value from 0 to max
unsigned int Rand( unsigned int max )
{
#ifdef _WIN32
	return rand()%max;
#else
	unsigned int seed = (unsigned int)System::clock();
	return rand_r(&seed)%max;
#endif
}

void randomGems()
{
    VidMode_BG0 vid(cube.vbuf);

    vid.setWindow(0, 128);
    vid.BG0_setPanning(Vec2(0,0));

    for (unsigned frames = 0; frames < 260; frames++) {
        for (unsigned x = 0; x < 16; x += 2)
            for (unsigned y = 0; y < 16; y += 2)
                vid.BG0_drawAsset(Vec2(x,y), Gems, Rand(Gems.frames));

        vid.set();
        System::paint();
    }
}


void siftmain()
{
    load();

    while (1) {
        whiteFlash();
        panToEarth();

        whiteFlash();
        earthZoomSlow();

        whiteFlash();
        lbZoom(Globe128, 30, 20, 0.5f, 2.5f);

        whiteFlash();
        lbZoom(Clouds, 74, 44, 0.5f, 2.0f);

        whiteFlash();
        lbZoom(Flower, 74, 28, 0.5f, 3.0f);

        whiteFlash();
        showMonsters();

        whiteFlash();
        owlbearRun();

        whiteFlash();
        randomGems();

        whiteFlash();
        brickScroll();
    }
}
