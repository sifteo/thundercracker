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

Game::Game() : m_bTestMatches( false ), m_iGemScore ( 0 ), m_iScore( 0 ), m_state( STATE_SPLASH ), m_mode( MODE_FLIPS ), m_splashTime( 0.0f )
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

		if( IsAllQuiet() )
			m_iGemScore = 0;
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


bool Game::IsAllQuiet()
{
	for( int i = 0; i < NUM_CUBES; i++ )
	{
		if( !cubes[i].IsQuiet() )
			return false;
	}

	return true;
}


