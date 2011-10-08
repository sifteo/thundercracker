/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cppfix.h"
#include "game.h"
#include "utils.h"
#include "assets.gen.h"


Game &Game::Inst()
{
	static Game game = Game();
	return game; 
}

Game::Game() : m_bTestMatches( false )
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
}


void Game::Update()
{
	if( m_bTestMatches )
	{
		TestMatches();
		m_bTestMatches = false;
	}

	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].Update();

	for( int i = 0; i < NUM_CUBES; i++ )
		cubes[i].Draw();
            
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