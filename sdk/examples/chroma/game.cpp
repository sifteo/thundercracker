/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "game.h"
#include "utils.h"
#include "assets.gen.h"
#include <stdlib.h>

Game &Game::Inst()
{
	static Game game = Game();
	return game; 
}

Game::Game() : m_bTestMatches( false ), m_iDotScore ( 0 ), m_iDotScoreSum( 0 ), m_iScore( 0 ), m_iDotsCleared( 0 ), m_state( STARTING_STATE ), m_mode( MODE_FLIPS ), m_splashTime( 0.0f )
{
}


void Game::Init()
{
	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].Init(GameAssets);

	bool done = false;

	PRINT( "getting ready to load" );

	while( !done )
	{
		done = true;
		for( int i = 0; i < NUM_CUBES; i++ )
		{
			if( !cubes[i].DrawProgress(GameAssets) )
				done = false;

			PRINT( "in load loop" );
		}
		System::paint();
	}
	PRINT( "done loading" );
	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].vidInit();

	m_splashTime = System::clock();
}


void Game::Update()
{
	if( m_state == STATE_SPLASH )
	{
		for( int i = 0; i < NUM_CUBES; i++ )
			cubes[i].DrawSplash();

		if( System::clock() - m_splashTime > 3.0f )
			m_state = STATE_PLAYING;
	}
	else if( m_state == STATE_PLAYING )
	{
		if( m_bTestMatches )
		{
			TestMatches();
			m_bTestMatches = false;
		}

		for( int i = 0; i < NUM_CUBES; i++ )
			cubes[i].Update( System::clock() );

		for( int i = 0; i < NUM_CUBES; i++ )
			cubes[i].Draw();
	}
            
    System::paint();
}


void Game::TestMatches()
{
	//for every cube test matches with every other cube
	for( int i = 0; i < NUM_CUBES; i++ )
	{
		cubes[i].testMatches();
	}
}



//get random value from 0 to max
unsigned int Game::Rand( unsigned int max )
{
#ifdef _WIN32
	return rand()%max;
#else
	static unsigned int seed = (int)System::clock();
	return rand_r(&seed)%max;
#endif
}



void Game::CheckChain( CubeWrapper *pWrapper )
{
	int total_marked = 0;

	for( int i = 0; i < NUM_CUBES; i++ )
	{
		total_marked += cubes[i].getNumMarked();
	}

    if( total_marked == 0 )
	{
		m_iScore += m_iDotScoreSum;
		m_iDotsCleared += m_iDotScore;

		if( m_mode == MODE_PUZZLE )
		{
			//TODO puzzle mode
			//check_puzzle();
		}
		else
		{
			//TODO sound
			/*# play sound based on number of gems cleared in this combo.
			dings = (
					(10, "score5"),
					(7, "score4"),
					(4, "score3"),
					(2, "score2"),
					(0, None),
					)
			sound = reduce(lambda x,y: cleared >= x[0] and x or y, dings)[1]
			if sound:
				self.sound_manager.add(sound)*/

			char aBuf[16];
			sprintf( aBuf, "%d", m_iDotScoreSum );
			pWrapper->getBanner().SetMessage( aBuf, Banner::SCORE_FADE_DELAY/2.0f );
		}

		//TODO timer mode
		/*if( m_mode == MODE_TIME )
			self.timekeeper.add_time(m_iDotsCleared * gems_timer.TIME_RETURN_PER_GEM)*/

		m_iDotScore = 0;
		m_iDotScoreSum = 0;
	}
}
