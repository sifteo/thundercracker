/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "GameOver.h"
#include "string.h"
#include "assets.gen.h"
#include "audio.gen.h"
#include "sprite.h"
#include "game.h"

GameOver::GameOver()
{
    Reset();
}


void GameOver::Reset()
{
    m_frame = 0;
}


void GameOver::Update( float dt )
{
    m_frame++;

    unsigned int lastFrame = ( NUM_ARROWS - 1 ) * FRAMES_BETWEEN_ANIMS + DieUp.frames;

    if( m_frame >= lastFrame )
    {
        Game::Inst().setState( Game::STATE_POSTGAME );
        Game::Inst().playSound(game_over);
        Reset();
    }
}

const Sifteo::PinnedAssetImage *DIE_SPRITES[ GameOver::NUM_ARROWS ] =
{
    &DieUp,
    &DieLeft,
    &DieDown,
    &DieRight,
};

Vec2 SPRITEPOS[ Intro::NUM_ARROWS ] = {
    Vec2( 64 - DieUp.width * 8 / 2, 0 ),
    Vec2( 0, 64 - DieLeft.height * 8 / 2 ),
    Vec2( 64 - DieDown.width * 8 / 2, 128 - DieDown.height * 8 ),
    Vec2( 128 - DieRight.width * 8, 64 - DieLeft.height * 8 / 2 ),
};

void GameOver::Draw( BG1Helper &bg1helper, Cube &cube )
{
    bg1helper.Clear();
    bg1helper.Flush();

    //arrow sprites
    for( int i = 0; i < NUM_ARROWS; i++ )
    {
        int frame = m_frame - ( i * FRAMES_BETWEEN_ANIMS );

        if( frame < 0 )
            frame = 0;
        else if ( frame >= (int)DIE_SPRITES[i]->frames )
            frame = DIE_SPRITES[i]->frames;

        resizeSprite(cube, i, DIE_SPRITES[i]->width*8, DIE_SPRITES[i]->height*8);
        setSpriteImage(cube, i, *DIE_SPRITES[i], frame);
        moveSprite(cube, i, SPRITEPOS[i].x, SPRITEPOS[i].y);
    }
}


