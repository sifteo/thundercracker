/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * timer for the game
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "GameOver.h"
#include "assets.gen.h"
//#include "audio.gen.h"
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
        Game::Inst().forcePaintSync();
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

Int2 SPRITEPOS[ Intro::NUM_ARROWS ] = {
    { 64 - DieUp.width * 8 / 2, 0 },
    { 0, 64 - DieLeft.height * 8 / 2 },
    { 64 - DieDown.width * 8 / 2, 128 - DieDown.height * 8 },
    { 128 - DieRight.width * 8, 64 - DieLeft.height * 8 / 2 },
};

void GameOver::Draw( VidMode_BG0_SPR_BG1 &vid )
{
    //arrow sprites
    for( int i = 0; i < NUM_ARROWS; i++ )
    {
        int frame = m_frame - ( i * FRAMES_BETWEEN_ANIMS );

        if( frame < 0 )
            frame = 0;
        else if ( frame >= (int)DIE_SPRITES[i]->frames )
            frame = DIE_SPRITES[i]->frames;

        vid.resizeSprite(i, DIE_SPRITES[i]->width*8, DIE_SPRITES[i]->height*8);
        vid.setSpriteImage(i, *DIE_SPRITES[i], frame);
        vid.moveSprite(i, SPRITEPOS[i].x, SPRITEPOS[i].y);
    }
}


