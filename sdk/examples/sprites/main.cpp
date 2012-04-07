/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("Sprites SDK Example")
    .cubeRange(1);

static const CubeID cube(0);
static VideoBuffer vid;

void main()
{
    cube.enable();
    vid.initMode(BG0_SPR_BG1);
    vid.attach(cube);

    vid.bg0.image(vec(0,0), Background);

    // Allocate sprite IDs
    SpriteRef sBullet = vid.sprites[0];
    SpriteRef s1UP = vid.sprites[1];
    const unsigned num1UPs = vid.sprites.NUM_SPRITES - 1;

    // 1UPs
    for (unsigned i = 0; i < num1UPs; i++)
        s1UP[i].setImage(Sprite);

    // Bullet
    sBullet.setImage(Bullet);

#if 0
    // BG1 Overlay
    _SYS_vbuf_fill(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2
                   + 16 - Overlay.height,
                   ((1 << Overlay.width) - 1), Overlay.height);
    _SYS_vbuf_writei(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_tiles) / 2,
                     Overlay.tiles, 0, Overlay.width * Overlay.height);
#endif

    int frame = 0;
    while (1) {
        frame++;

        // Circle of 1UPs
        for (unsigned i = 0; i < num1UPs; i++) {

            float angle = frame * 0.075f + i * (M_PI*2 / num1UPs);
            const float r = 32;
            const Float2 center = { (LCD_width - Sprite.pixelWidth()) / 2,
                                    (LCD_height - Sprite.pixelHeight()) / 2 };

            s1UP[i].move(center + polar(angle, r));
        }

        // Scroll BG1
        //vid.BG1_setPanning(vec(-frame, 0u));
        
        // Flying bullet
        const Int2 bulletOrigin = { 130, 190 };
        const Int2 bulletDelta = { -3, -1 };
        sBullet.move(bulletOrigin + frame * bulletDelta);

        System::paint();
    }
}
