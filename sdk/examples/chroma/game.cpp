/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "game.h"
#include "utils.h"
#include "assets.gen.h"

//TODO, load this from save file
unsigned int Game::s_HighScores[ Game::NUM_HIGH_SCORES ] =
        { 1000, 800, 600, 400, 200 };


const float Game::SLOSH_THRESHOLD = 0.4f;

Math::Random Game::random;


Game &Game::Inst()
{
	static Game game = Game();

    return game;
}

Game::Game() : m_bTestMatches( false ), m_iDotScore ( 0 ), m_iDotScoreSum( 0 ), m_iScore( 0 ), m_iDotsCleared( 0 ),
                m_state( STARTING_STATE ), m_mode( MODE_SHAKES ), m_splashTime( 0.0f ),
                m_fLastSloshTime( 0.0f ), m_curChannel( 0 ), m_pSoundThisFrame( NULL ),
                m_ShakesRemaining( STARTING_SHAKES ), m_bForcePaintSync( false )//, m_bHyperDotMatched( false ),
                , m_bStabilized( false )
{
	//Reset();
}


void Game::Init()
{
	for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].Init(GameAssets);

#if LOAD_ASSETS
    bool done = false;

	PRINT( "getting ready to load" );

	while( !done )
	{
		done = true;
		for( int i = 0; i < NUM_CUBES; i++ )
		{
            if( !m_cubes[i].DrawProgress(GameAssets) )
				done = false;

			PRINT( "in load loop" );
		}
		System::paint();
	}
    PRINT( "done loading" );
#endif
    for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].Reset();

	for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].vidInit();

	m_splashTime = System::clock();
    m_fLastTime = m_splashTime;

#if SFX_ON
    for( unsigned int i = 0; i < NUM_SFX_CHANNELS; i++ )
    {
        m_SFXChannels[i].init();
        //m_SFXChannels[i].setVolume( 256 );
    }
#endif
#if MUSIC_ON
    m_musicChannel.init();

    //doesn't seem to work
    m_musicChannel.setVolume( 1 );
#if SPLASH_ON
    m_musicChannel.play( StingerIV2, LoopOnce );
#else
    m_musicChannel.play( astrokraut, LoopRepeat );
#endif
#endif
}


void Game::Update()
{
    float t = System::clock();
    float dt = t - m_fLastTime;
    m_fLastTime = t;

    bool needsync = false;

    if( m_bForcePaintSync )
    {
        needsync = true;
        System::paintSync();
        m_bForcePaintSync = false;
    }

	if( m_state == STATE_SPLASH )
	{
		for( int i = 0; i < NUM_CUBES; i++ )
            m_cubes[i].Draw();

        if( System::clock() - m_splashTime > 7.0f )
		{
            m_state = STATE_INTRO;
#if MUSIC_ON
            m_musicChannel.stop();
            m_musicChannel.play( astrokraut, LoopRepeat );
#endif
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

        if( m_state == STATE_PLAYING )
        {
            if( m_mode == MODE_TIMED )
            {
                m_timer.Update( dt );
                checkGameOver();
            }
            else if( m_mode == MODE_SHAKES )
            {
                if( m_bStabilized && m_ShakesRemaining == 0 && AreNoCubesEmpty() )
                    checkGameOver();

                m_bStabilized = false;
            }
        }

		for( int i = 0; i < NUM_CUBES; i++ )
            m_cubes[i].Update( System::clock(), dt );

        for( int i = 0; i < NUM_CUBES; i++ )
            m_cubes[i].Draw();

        //always finishing works
        //System::finish();
#if !SLOW_MODE
        //if any of our cubes have messed with bg1's bitmaps,
        //force a finish here
        for( int i = 0; i < NUM_CUBES; i++ )
        {
            if( m_cubes[i].getBG1Helper().NeedFinish() )
            {
                //System::finish();
                System::paintSync();
                needsync = true;
                //printf( "finishing\n" );
                break;
            }
        }
#endif
        for( int i = 0; i < NUM_CUBES; i++ )
            m_cubes[i].FlushBG1();
	}

#if SLOW_MODE
    System::paintSync();
#else
    if( needsync )
        System::paintSync();
    else
        System::paint();
#endif

    m_pSoundThisFrame = NULL;
}


void Game::Reset()
{
	m_iDotScore = 0;
	m_iDotScoreSum = 0;
	m_iScore = 0;
	m_iDotsCleared = 0;
	m_iLevel = 0;
    //m_bHyperDotMatched = false;

    m_state = STATE_INTRO;
    m_ShakesRemaining = STARTING_SHAKES;
    //m_musicChannel.play( astrokraut, LoopRepeat );

	for( int i = 0; i < NUM_CUBES; i++ )
	{
        m_cubes[i].Reset();
	}

	m_timer.Reset();

    m_bStabilized = false;
}

void Game::TestMatches()
{
	//for every cube test matches with every other cube
	for( int i = 0; i < NUM_CUBES; i++ )
	{
        m_cubes[i].testMatches();
	}
}

void Game::CheckChain( CubeWrapper *pWrapper )
{
	int total_marked = 0;

	for( int i = 0; i < NUM_CUBES; i++ )
	{
        total_marked += m_cubes[i].getNumMarked();
	}

    //chain is finished
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
            bool bannered = false;

            //free shake
            /*if( m_mode == MODE_SHAKES && m_iDotsCleared >= DOT_THRESHOLD5 && !m_bHyperDotMatched )
            {

            }
            else if( m_iDotsCleared >= DOT_THRESHOLD4 )
            {
                playSound(clear4);

                if( m_mode == MODE_SHAKES && !m_bHyperDotMatched )
                {
                    pWrapper->getBanner().SetMessage( "Bonus Shake!" );
                    bannered = true;
                    m_ShakesRemaining++;
                }
            }
            else */if( m_iDotsCleared >= DOT_THRESHOLD3 )
            {
                playSound(clear3);

                //is it dangerous to add one here?  do we need to queue it?
                if( /*!m_bHyperDotMatched && */!DoesHyperDotExist() )
                    pWrapper->SpawnHyper();
            }
            else if( m_iDotsCleared >= DOT_THRESHOLD2 )
                playSound(clear2);
            else if( m_iDotsCleared >= DOT_THRESHOLD1 )
                playSound(clear1);

            if( !bannered )
            {
                String<16> aBuf;
                aBuf << m_iDotScoreSum;
                pWrapper->getBanner().SetMessage( aBuf, true );
            }
		}

		if( m_mode == MODE_TIMED )
			m_timer.AddTime(m_iDotScore);

		m_iDotScore = 0;
		m_iDotScoreSum = 0;
        m_iDotsCleared = 0;

        //m_bHyperDotMatched = false;
        m_bStabilized = true;
	}
}


void Game::checkGameOver()
{
	if( m_mode == MODE_SHAKES )
	{
        if( NoMatches() )
            EndGame();
	}
	else if( m_mode == MODE_TIMED )
	{
		if( m_timer.getTime() <= 0.0f )
        {
            EndGame();
        }
	}
}


bool Game::NoMatches()
{
    if( DoesHyperDotExist() )
        return false;

    //shakes mode checks for no possible moves, whereas puzzle mode checks if the puzzle is lost
    if( m_mode == MODE_SHAKES )
    {
        if( AreAllColorsUnmatchable() )
            return true;
        if( DoCubesOnlyHaveStrandedDots() )
            return true;
    }
    else if( m_mode == MODE_PUZZLE )
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
    }

    return false;
}


unsigned int Game::numColors() const
{
	bool aColors[GridSlot::NUM_COLORS];

    _SYS_memset8( (uint8_t*)aColors, 0, sizeof( bool ) * GridSlot::NUM_COLORS );

	for( int i = 0; i < NUM_CUBES; i++ )
	{
        m_cubes[i].fillColorMap( aColors );
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
        if( NumCubesWithColor(i) == 1 )
            return true;
	}

	return false;
}


bool Game::AreAllColorsUnmatchable() const
{
    for( unsigned int i = 0; i < GridSlot::NUM_COLORS; i++ )
    {
        if( !IsColorUnmatchable(i) )
            return false;
    }

    return true;
}


//this only checks if a cube has a color that no other cubes have
bool Game::IsColorUnmatchable( unsigned int color ) const
{
    int total = 0;
    bool aHasColor[ NUM_CUBES ];

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].hasColor(color) )
        {
            total++;
            aHasColor[i] = true;
        }
        else
            aHasColor[i] = false;
    }

    if( total <= 1 )
        return true;

    int numCorners = 0;
    bool side1 = false;
    bool side2 = false;

    //also, make sure these colors on these cubes can possibly match
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( aHasColor[i] )
        {
            bool localCorners = false;
            bool localside1 = false;
            bool localside2 = false;

            m_cubes[i].UpdateColorPositions( color, localCorners, localside1, localside2 );

            if( localCorners )
            {
                numCorners++;
                //this color has corners on multiple cubes, there's a match!
                if( numCorners >= 2 )
                    return false;
            }

            //side1 on one cube can match side2 on another
            if( localside1 )
            {
                if( side2 )
                    return false;
            }
            if( localside2 )
            {
                if( side1 )
                    return false;
            }

            if( localside1 )
                side1 = true;
            if( localside2 )
                side2 = true;
        }
    }

    return true;
}

//return the number of cubes with the given color
unsigned int Game::NumCubesWithColor( unsigned int color ) const
{
    int total = 0;

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].hasColor(color) )
        {
            total++;
        }
    }

    return total;
}


bool Game::no_match_stranded_interior() const
{
    /*
    Returns true if the given grids can never match due to one of them having
    only fixed dots stranded in the middle.
    */
	for( int i = 0; i < NUM_CUBES; i++ )
	{
        if( m_cubes[i].hasStrandedFixedDots() )
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
        if( m_cubes[i].allFixedDotsAreStrandedSide() )
		{
            if( OnlyOneOtherCorner( &m_cubes[i] ) )
                return true;
		}
	}

	return false;   
}


bool Game::OnlyOneOtherCorner( const CubeWrapper *pWrapper ) const
{
    int numCorners = 0;

    for( int i = 0; i < NUM_CUBES; i++ )
    {

        if( &m_cubes[i] != pWrapper )
        {
            unsigned int thisCubeNumCorners = m_cubes[i].getNumCornerDots();
            numCorners += thisCubeNumCorners;
            if( numCorners > 1 || m_cubes[i].getNumDots() > thisCubeNumCorners )
                return false;
        }
    }

    if( numCorners == 1 )
        return true;

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
        if( m_cubes[i].getFixedDot( aBuddies[iNumBuddies] ) )
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

    //aren't there 2 sets missing here?

    return false;
}


bool Game::DoCubesOnlyHaveStrandedDots() const
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].hasNonStrandedDot() )
        {
            return false;
        }
    }

    return true;
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
    for( int i = (int)NUM_HIGH_SCORES - 1; i >= 0; i-- )
    {
        if( s_HighScores[i] < m_iScore )
        {
            if( i < (int)NUM_HIGH_SCORES - 1 )
            {
                s_HighScores[i+1] = s_HighScores[i];
            }
        }
        else
        {
            if( i < (int)NUM_HIGH_SCORES - 1 )
            {
                s_HighScores[i+1] = m_iScore;
            }

            break;
        }
    }
}


void Game::playSound( _SYSAudioModule &sound )
{
    if( &sound == m_pSoundThisFrame )
        return;

#if SFX_ON
    m_SFXChannels[m_curChannel].stop();
    m_SFXChannels[m_curChannel].play(sound, LoopOnce);
#endif

    //printf( "playing a sound effect %d on channel %d\n", sound.size, m_curChannel );

    m_curChannel++;

    if( m_curChannel >= NUM_SFX_CHANNELS )
        m_curChannel = 0;

    m_pSoundThisFrame = &sound;
}

_SYSAudioModule *SLOSH_SOUNDS[Game::NUM_SLOSH_SOUNDS] =
{
  &slosh_multi_01,
    &slosh_multi_02,
};


//play a random slosh sound
void Game::playSlosh()
{
    if( System::clock() - m_fLastSloshTime > SLOSH_THRESHOLD )
    {
        int index = random.randrange( NUM_SLOSH_SOUNDS );
        playSound(*SLOSH_SOUNDS[index]);

        m_fLastSloshTime = System::clock();
    }
}



//destroy all dots of the given color
void Game::BlowAll( unsigned int color )
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        m_cubes[i].BlowAll( color );
    }

    //m_bHyperDotMatched = true;
}



bool Game::DoesHyperDotExist()
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].HasHyperDot() )
            return true;
    }

    return false;
}


void Game::EndGame()
{
    enterScore();
    m_state = STATE_DYING;

    if( m_mode == MODE_SHAKES )
    {
        for( int i = 0; i < NUM_CUBES; i++ )
        {
            m_cubes[i].getBanner().SetMessage( "NO MORE MATCHES" );
        }

    }

    playSound(timer_explode);
}

bool Game::AreNoCubesEmpty() const
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].isEmpty() )
            return false;
    }

    return true;
}

