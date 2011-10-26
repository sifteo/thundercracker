/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include <stdlib.h>
#include "assets.gen.h"

using namespace Sifteo;

static Cube cube(0);

static struct {
    float x, y, vx, vy;
} stars[8];

    
unsigned int randint(unsigned int max)
{
#ifdef _WIN32
	return rand()%max;
#else
	static unsigned int seed = (int)System::clock();
	return rand_r(&seed)%max;
#endif
}
    
void load()
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

void moveSprite(int id, int x, int y)
{
    uint8_t xb = -x;
    uint8_t yb = -y;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void resizeSprite(int id, int w, int h)
{
    uint8_t xb = -w;
    uint8_t yb = -h;

    uint16_t word = ((uint16_t)xb << 8) | yb;
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void setSpriteImage(int id, int tile)
{
    uint16_t word = VideoBuffer::indexWord(tile);
    uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                      sizeof(_SYSSpriteInfo)/2 * id ); 

    _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
}

void panBG1(int8_t x, int8_t y)
{
    _SYS_vbuf_poke(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_x) / 2,
                   ((uint8_t)x) | ((uint16_t)(uint8_t)y << 8));
}

void text(const char *str)
{
    // Text on BG1, in the 16x2 area we allocated
    uint16_t addr = offsetof(_SYSVideoRAM, bg1_tiles) / 2;
    char c;
    
    while ((c = *(str++))) {
        uint16_t index = (c - ' ') * 2 + Font.index;
        cube.vbuf.pokei(addr, index);
        cube.vbuf.pokei(addr + 16, index + 1);
        addr++;
    }
}

void initStar(int id)
{
    stars[id].x = 0;
    stars[id].y = 0;
    
    float angle = randint(1000) * (2 * M_PI / 1000.0f);
    float speed = (randint(100) + 50) * 0.01f;
    
    stars[id].vx = cosf(angle) * speed;
    stars[id].vy = sinf(angle) * speed;
}
 
void siftmain()
{
    load();
    
    VidMode_BG0 vid(cube.vbuf);
    vid.init();
    vid.BG0_drawAsset(Vec2(0,0), Background);
 
    // Clear BG1/SPR before switching modes
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_TILES/2,
                   cube.vbuf.indexWord(Font.index), 32);
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
    
    // Allocate 16x2 tiles on BG1 for text at the bottom of the screen
    _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2 + 14, 0xFFFF, 2);
    
    // Start it out scrolled off the bottom edge
    panBG1(0, -16);

    // Switch modes
    _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
        
    float lastTime = System::clock();
    unsigned frame = 0;
    float bg_x = 0;
    float bg_y = 0;
    float text_x = 0;
    float text_y = -20;
    float text_tx = text_x;
    float text_ty = text_y;
        
    for (unsigned i = 0; i < 8; i++)
        initStar(i);
    
    while (1) {
        float now = System::clock();
        float dt = now - lastTime;
        lastTime = now;
       
        /*
         * Action triggers
         */
        
        switch (frame) {
        
        case 100:
            text(" Hello traveler ");
            text_ty = 8;
            break;

        case 200:
            text_ty = -20;
            break;
            
        case 250:
            text(" Tilt me to fly ");
            text_ty = 8;
            break;
            
        case 350:
            text_ty = -20;
            break;
        
        case 400:
            text(" Enjoy subspace ");
            text_ty = 64-8;
            break;
        
        case 500:
            text_tx = 128;
            break;
        
        case 550:
            text_x = 128;
            text_tx = 0;
            text(" and be careful ");
            break;

        case 650:
            text_ty = -20;
            break;

        case 2048:
            frame = 0;
            break;
        }
       
        /*
         * We respond to the accelerometer
         */
         
        _SYSAccelState state;
        _SYS_getAccel(0, &state);
        float vx = state.x * 0.1f;
        float vy = state.y * 0.1f;

        /*
         * Update starfield animation
         */
        
        for (unsigned i = 0; i < 8; i++) {
            resizeSprite(i, 8, 8);
            setSpriteImage(i, Star.index + (frame % Star.frames) * Star.width * Star.height);
            moveSprite(i, stars[i].x + (64 - 3.5f), stars[i].y + (64 - 3.5f));
            stars[i].x += stars[i].vx + vx;
            stars[i].y += stars[i].vy + vy;
            
            if (stars[i].x > 80 || stars[i].x < -80 ||
                stars[i].y > 80 || stars[i].y < -80)
                initStar(i);
        }
        
        /*
         * Update global animation
         */

        frame++;
        
        bg_y += dt * -10.0f + vy * 0.1f;
        bg_x += vx * 0.1f;
        vid.BG0_setPanning(Vec2(bg_x + 0.5f, bg_y + 0.5f));
        
        text_x += (text_tx - text_x) * 0.2f;
        text_y += (text_ty - text_y) * 0.2f;
        panBG1(text_x, text_y);
        
        System::paint();
    }
}
