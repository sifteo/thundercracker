/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "game.h"
#include "utils.h"
#include "assets.gen.h"
#include "string.h"
#include <stdlib.h>

//TODO, load this from save file
unsigned int Game::s_HighScores[ Game::NUM_HIGH_SCORES ] =
        { 1000, 800, 600, 400, 200 };

Game &Game::Inst()
{
	static Game game = Game();
	return game; 
}

Game::Game() : m_bTestMatches( false ), m_iDotScore ( 0 ), m_iDotScoreSum( 0 ), m_iScore( 0 ), m_iDotsCleared( 0 ), m_state( STARTING_STATE ), m_mode( MODE_TIMED ), m_splashTime( 0.0f )
{
	//Reset();
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
    m_fLastTime = m_splashTime;
}


void Game::Update()
{
    float t = System::clock();
    float dt = t - m_fLastTime;
    m_fLastTime = t;

	if( m_state == STATE_SPLASH )
	{
		for( int i = 0; i < NUM_CUBES; i++ )
			cubes[i].Draw();

		if( System::clock() - m_splashTime > 3.0f )
		{
            m_state = STATE_INTRO;
			m_timer.Init( System::clock() );
		}
	}
	else 
	{
		if( m_bTestMatches )
		{
			if( m_state == STATE_PLAYING )
				TestMatches();
			else
			{
				Reset();
			}
			m_bTestMatches = false;
		}

        if( m_mode == MODE_TIMED && m_state == STATE_PLAYING )
		{
            m_timer.Update( dt );
			checkGameOver();
		}

		for( int i = 0; i < NUM_CUBES; i++ )
            cubes[i].Update( System::clock(), dt );

		for( int i = 0; i < NUM_CUBES; i++ )
			cubes[i].Draw();
	}
            
    System::paint();
}


void Game::Reset()
{
	m_iDotScore = 0;
	m_iDotScoreSum = 0;
	m_iScore = 0;
	m_iDotsCleared = 0;
	m_iLevel = 0;

	m_state = STARTING_STATE;

	for( int i = 0; i < NUM_CUBES; i++ )
	{
		cubes[i].Reset();
	}

	m_timer.Reset();
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
		if( m_mode == MODE_TIMED )
			m_timer.AddTime(m_iDotScore);

		m_iDotScore = 0;
		m_iDotScoreSum = 0;
	}
}


void Game::checkGameOver()
{
	if( m_mode == MODE_SHAKES )
	{
		//1 or fewer cubes not dead 
		int numInPlay = 0;

		for( int i = 0; i < NUM_CUBES; i++ )
		{
			if( !cubes[i].isDead() )
				numInPlay++;
		}

		if( numInPlay <= 1 )
        {
            enterScore();
			m_state = STATE_POSTGAME;
        }
	}
	else if( m_mode == MODE_TIMED )
	{
		if( m_timer.getTime() <= 0.0f )
        {
            enterScore();
            m_state = STATE_POSTGAME;
        }
	}
}


bool Game::NoMatches()
{
    //""" Return True if no matches are possible with the current gems. """
    if( no_match_color_imbalance() )
		return true;
    if( numColors() == 1 )
	{
        if( no_match_stranded_interior() )
            return true;
        else if( no_match_stranded_side() )
            return true;
        else if( no_match_mismatch_side() )
            return true;
	}

    return false;
}


unsigned int Game::numColors() const
{
	bool aColors[GridSlot::NUM_COLORS];

	memset( aColors, 0, sizeof( bool ) * GridSlot::NUM_COLORS );

	for( int i = 0; i < NUM_CUBES; i++ )
	{
		cubes[i].fillColorMap( aColors );
	}
    
	int numColors = 0;

	for( unsigned int i = 0; i < GridSlot::NUM_COLORS; i++ )
	{
		if( aColors[i] )
			numColors++;
	}
    
	return numColors;
}

bool Game::no_match_color_imbalance() const
{
    /*
    Returns true if all the dots of a given color are stuck together on
    one grid, meaning that they can never be matched.
    */
	for( unsigned int i = 0; i < GridSlot::NUM_COLORS; i++ )
	{
		int total = 0;

		for( int i = 0; i < NUM_CUBES; i++ )
		{
			if( cubes[i].hasColor(i) )
				total++;
		}

		if( total == 1 )
			return true;
	}

	return false;
}

bool Game::no_match_stranded_interior() const
{
    /*
    Returns true if the given grids can never match due to one of them having
    only fixed dots stranded in the middle.
    */
	for( int i = 0; i < NUM_CUBES; i++ )
	{
		if( cubes[i].hasStrandedFixedDots() )
			return true;
	}

	return false;
}

bool Game::no_match_stranded_side() const
{
    /*
    Returns true if one of the given grids has only fixed dots on a
    side-center (i.e., not a corner), but no floating dots can match it.
    */

	for( int i = 0; i < NUM_CUBES; i++ )
	{
		if( cubes[i].allFixedDotsAreStrandedSide() )
		{
			int numCorners = 0;

			for( int j = 0; j < NUM_CUBES; j++ )
			{
				
				if( i != j )
				{
					unsigned int thisCubeNumCorners = cubes[j].getNumCornerDots();
					numCorners += thisCubeNumCorners;
					if( numCorners > 1 || cubes[j].getNumDots() > thisCubeNumCorners )
						break;
				}
			}

			if( numCorners == 1 )
				return true;
		}
	}

	return false;   
}


bool Game::no_match_mismatch_side() const
{
    /*
    Returns true if the given grids have one fixed gem on a
    side, but the gems can never touch.
    */

	Vec2 aBuddies[3];
	int iNumBuddies = 0;

	for( int i = 0; i < NUM_CUBES; i++ )
	{
		if( cubes[i].getFixedDot( aBuddies[iNumBuddies] ) )
		{
			iNumBuddies++;
			if( iNumBuddies > 2 )
				return false;
		}
	}

	int SIDE_MISMATCH_SET1[] =  { 2, 0, 3, 1 };
	int SIDE_MISMATCH_SET2[] =  { 1, 3, 0, 2 };

	if( SIDE_MISMATCH_SET1[aBuddies[0].x] == aBuddies[0].y && SIDE_MISMATCH_SET1[aBuddies[1].x] == aBuddies[1].y )
		return true;
	if( SIDE_MISMATCH_SET2[aBuddies[0].x] == aBuddies[0].y && SIDE_MISMATCH_SET2[aBuddies[1].x] == aBuddies[1].y )
		return true;

    return false;
}


unsigned int Game::getHighScore( unsigned int index ) const
{
    ASSERT( index < NUM_HIGH_SCORES );

    if( index < NUM_HIGH_SCORES )
        return s_HighScores[ index ];
    else
        return 0;
}



void Game::enterScore()
{
    //walk backwards through the high score list and see which ones we can pick off
    for( unsigned int i = NUM_HIGH_SCORES - 1; i >= 0; i-- )
    {
        if( s_HighScores[i] < m_iScore )
        {
            if( i < NUM_HIGH_SCORES - 1 )
            {
                s_HighScores[i+1] = s_HighScores[i];
            }
        }
        else
        {
            if( i < NUM_HIGH_SCORES - 1 )
            {
                s_HighScores[i+1] = m_iScore;
            }

            break;
        }
    }
}
