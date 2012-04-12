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

    // BG1 Overlay
    const Int2 ovlPos = vid.bg1.tileSize() - Overlay.tileSize();
    vid.bg1.setMask(BG1Mask::filled(ovlPos, Overlay.tileSize()));
    vid.bg1.image(ovlPos, Overlay);

    SystemTime epoch = SystemTime::now();
    while (1) {
        float t = SystemTime::now() - epoch;

        // Circle of 1UPs
        for (unsigned i = 0; i < num1UPs; i++) {
            float angle = t * 2.f + i * (M_PI*2 / num1UPs);
            s1UP[i].move(LCD_center - Sprite.pixelExtent() + polar(angle, 32.f));
        }

        // Scroll BG1
        vid.bg1.setPanning(t * vec(-10.f, 0.f));
        
        // Flying bullet
        sBullet.move(vec(130.f, 190.f) + t * polar(0.5f, -50.f));

        System::paint();
    }
}
