/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo/menu.h>
#include "game.h"
#include "utils.h"
#include "assets.gen.h"
#include "Puzzle.h"
#include "BubbleTransition.h"
#include "Banner.h"
#include "SpriteNumber.h"

//TODO, load this from save file
unsigned int Game::s_HighScores[ Game::NUM_HIGH_SCORES ] =
        { 1000, 800, 600, 400, 200 };

unsigned int Game::s_HighCubes[ Game::NUM_HIGH_SCORES ] =
        { 20, 10, 8, 6, 4 };


const float Game::SLOSH_THRESHOLD = 0.4f;
const float Game::TIME_TO_RESPAWN = 0.55f;
const float Game::COMBO_TIME_THRESHOLD = 2.5f;
const float Game::LUMES_FACE_TIME = 2.0f;
const float Game::FULLGOODJOB_TIME = 4.0f;


Random Game::random;


Game &Game::Inst()
{
	static Game game = Game();

    return game;
}

Game::Game() : m_bTestMatches( false ), m_iDotScore ( 0 ), m_iDotScoreSum( 0 ), m_iScore( 0 ), m_iDotsCleared( 0 ),
                m_state( STARTING_STATE ), m_mode( MODE_SURVIVAL ), m_stateTime( 0.0f ),
                m_lastSloshTime(), m_curChannel( 0 ), m_pSoundThisFrame( NULL ),
                m_ShakesRemaining( STARTING_SHAKES ), m_fTimeTillRespawn( TIME_TO_RESPAWN ),
                m_cubeToRespawn ( 0 ), m_comboCount( 0 ), m_fTimeSinceCombo( 0.0f ),
                m_Multiplier(1), m_bForcePaintSync( false )//, m_bHyperDotMatched( false ),
  , m_bStabilized( false ), m_bIsChainHappening( false )
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
    Reset( false );

    for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].Reset();

	for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].vidInit();

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

    m_stateTime = 0.0f;

    //TODO READ THIS FROM SAVE FILE
    m_iFurthestProgress = 30;
    m_iChapterViewed = 0;
}


void Game::Update()
{
    m_timeStep.next();
    SystemTime t = m_timeStep.end();
    TimeDelta dt = m_timeStep.delta();
    m_stateTime += dt;

    bool needsync = false;

    //painting is handled by menu manager
    switch( m_state )
    {
        case STATE_MAINMENU:
        case STATE_PUZZLEMENU:
        case STATE_CHAPTERSELECTMENU:
        case STATE_PUZZLESELECTMENU:
        {
            HandleMenu();
            return;
        }
        default:
            break;
    }

    if( m_bForcePaintSync )
    {
        needsync = true;
        System::paintSync();
        m_bForcePaintSync = false;
    }

    switch( m_state )
    {
        case STATE_SPLASH:
        {
            if( m_stateTime > 7.0f )
            {
                setState( STATE_INTRO );
    #if MUSIC_ON
                m_musicChannel.stop();
                m_musicChannel.play( astrokraut, LoopRepeat );
    #endif
                m_timer.Init( SystemTime::now() );
            }
            break;
        }
        case STATE_GOODJOB:
        {
            if( m_stateTime > FULLGOODJOB_TIME )
            {
                gotoNextPuzzle( true );
            }
            break;
        }
        case STATE_GAMEOVERBANNER:
        {
            if( !m_cubes[0].getBanner().IsActive() )
                TransitionToState( STATE_POSTGAME );
            break;
        }
        default:
        {
            if( m_bTestMatches )
            {
                TestMatches();

                if( m_state == STATE_POSTGAME || m_state == STATE_GAMEMENU )
                    Reset();

                m_bTestMatches = false;
            }

            if( m_state == STATE_PLAYING )
            {
                if( m_mode == MODE_BLITZ )
                {
                    m_timer.Update( dt );
                    checkGameOver();

                    m_fTimeSinceCombo += dt;

                    if( m_fTimeSinceCombo > COMBO_TIME_THRESHOLD )
                        m_comboCount = 0;

                    if( !m_bIsChainHappening )
                    {
                        m_fTimeTillRespawn -= dt;

                        if( m_fTimeTillRespawn <= 0.0f )
                            RespawnOnePiece();
                    }
                }
                else if( m_mode == MODE_SURVIVAL )
                {
                    if( m_bStabilized && m_ShakesRemaining == 0 && AreNoCubesEmpty() )
                        checkGameOver();

                    m_bStabilized = false;
                }
            }
            break;
        }

    }

    for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].Update( t, dt );

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


void Game::Reset(  bool bInGame )
{
	m_iDotScore = 0;
	m_iDotScoreSum = 0;
	m_iScore = 0;
	m_iDotsCleared = 0;

    //m_bHyperDotMatched = false;

    if( bInGame )
        TransitionToState( STATE_INTRO );
    else
        setState( STARTING_STATE );
    m_ShakesRemaining = STARTING_SHAKES;
    //m_musicChannel.play( astrokraut, LoopRepeat );

	for( int i = 0; i < NUM_CUBES; i++ )
	{
        m_cubes[i].Reset();
	}

    for( unsigned int i = 0; i < GridSlot::NUM_COLORS; i++ )
    {
        m_aColorsUsed[i] = false;
    }

	m_timer.Reset();
    m_fTimeTillRespawn = TIME_TO_RESPAWN;
    m_comboCount = 0;
    m_fTimeSinceCombo = 0.0f;
    m_Multiplier = 1;

    m_bStabilized = false;
    m_bIsChainHappening = false;
}


CubeWrapper *Game::GetWrapper( Cube *pCube )
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( pCube == &m_cubes[i].GetCube() )
            return &m_cubes[i];
    }

    return NULL;
}


CubeWrapper *Game::GetWrapper( unsigned int index )
{
    return &m_cubes[index];
}


int Game::getWrapperIndex( const CubeWrapper *pWrapper )
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( &m_cubes[i] == pWrapper )
            return i;
    }

    return -1;
}


void Game::setState( GameState state )
{
    m_state = state;
    m_stateTime = 0.0f;

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        m_cubes[i].setDirty();
    }

    if( m_state == STATE_INTRO )
    {
        for( int i = 0; i < NUM_CUBES; i++ )
        {
            m_cubes[i].resetIntro();
        }
    }
}


//go to state through bubble transition
void Game::TransitionToState( GameState state )
{
    DoBubbleTransition();
    setState( state );
}


unsigned int Game::getScore() const
{
    if( m_mode == MODE_BLITZ )
        return m_iScore;
    else
        return getDisplayedLevel();
}


void Game::TestMatches()
{
    if( !Game::Inst().AreMovesLegal() )
        return;

	//for every cube test matches with every other cube
	for( int i = 0; i < NUM_CUBES; i++ )
	{
        m_cubes[i].testMatches();
	}
}

void Game::CheckChain( CubeWrapper *pWrapper, const Int2 &slotPos )
{
	int total_marked = 0;

	for( int i = 0; i < NUM_CUBES; i++ )
	{
        total_marked += m_cubes[i].getNumMarked();
	}

    //chain is finished
    if( total_marked == 0 )
	{
        unsigned int comboScore = m_iDotScoreSum;

        if( m_mode == MODE_BLITZ )
        {
            comboScore += 10 * m_comboCount;
            comboScore *= m_Multiplier;
        }

        m_iScore += comboScore;
		m_iDotsCleared += m_iDotScore;

		if( m_mode == MODE_PUZZLE )
		{
            check_puzzle();
		}
		else
        {
            bool bannered = false;
            bool specialSpawned = false;
            int numColors = 0;

            //count how many colors we used for this combo
            for( unsigned int i = 0; i < GridSlot::NUM_COLORS; i++ )
            {
                if( m_aColorsUsed[i] )
                {
                    numColors++;
                    m_aColorsUsed[i] = false;
                }
            }

            if( numColors >= NUM_COLORS_FOR_HYPER )
            {
                pWrapper->SpawnSpecial( GridSlot::HYPERCOLOR );
                specialSpawned = true;
            }

            if( !specialSpawned )
            {
                if( m_mode == MODE_SURVIVAL )
                {
                    if( m_iDotsCleared >= DOT_THRESHOLD3 )
                    {
                        playSound(clear3);

                        //is it dangerous to add one here?  do we need to queue it?
                        //if( !m_bHyperDotMatched && !DoesHyperDotExist() )
                        pWrapper->SpawnSpecial( GridSlot::RAINBALLCOLOR );
                        specialSpawned = true;
                    }
                }
                else if( m_mode == MODE_BLITZ )
                {
                    if( m_iDotsCleared >= DOT_THRESHOLD_TIMED_MULT && m_Multiplier < MAX_MULTIPLIER - 1 )
                    {
                        playSound(clear4);

                        if( !pWrapper->SpawnMultiplier( m_Multiplier + 1 ) )
                        {
                            //find another cube to spawn multiplier on
                            for( int i = 0; i < NUM_CUBES; i++ )
                            {
                                if( &m_cubes[i] != pWrapper )
                                {
                                    if( m_cubes[i].SpawnMultiplier( m_Multiplier + 1 ) )
                                        break;
                                }
                            }
                        }
                        specialSpawned = true;
                    }
                    else if( m_iDotsCleared >= DOT_THRESHOLD_TIMED_RAINBALL )
                    {
                        playSound(clear3);

                        pWrapper->SpawnSpecial( GridSlot::RAINBALLCOLOR );
                        specialSpawned = true;
                    }
                }

                if( !specialSpawned )
                {
                    if( m_iDotsCleared >= DOT_THRESHOLD2 )
                        playSound(clear2);
                    else if( m_iDotsCleared >= DOT_THRESHOLD1 )
                        playSound(clear1);
                }
            }

            /*if( m_mode == MODE_BLITZ && !bannered )
            {
                String<16> aBuf;
                aBuf << comboScore;
                pWrapper->getBanner().SetMessage( aBuf, Banner::SCORE_TIME, true );
            }*/

            if( m_mode == MODE_BLITZ )
            {
                SetChain( false );
                pWrapper->SpawnScore( comboScore, slotPos );
            }
		}

		m_iDotScore = 0;
		m_iDotScoreSum = 0;
        m_iDotsCleared = 0;

        //m_bHyperDotMatched = false;
        m_bStabilized = true;
	}
}


void Game::checkGameOver()
{
    if( m_mode == MODE_SURVIVAL )
	{
        if( NoMatches() )
            EndGame();
	}
    else if( m_mode == MODE_BLITZ )
	{
        if( m_timer.getTime() <= 0.0f && !m_bIsChainHappening )
        {
            EndGame();
        }
	}
}


bool Game::NoMatches()
{
    ASSERT( m_mode == MODE_SURVIVAL || m_mode == MODE_PUZZLE );
    //shakes mode checks for no possible moves, whereas puzzle mode checks if the puzzle is lost
    if( m_mode == MODE_PUZZLE )
    {
        //""" Return True if no matches are possible with the current gems. """
        if( no_match_color_imbalance() )
        {
            //LOG(("Imbalance!\n"));
            return true;
        }
        /*if( numColors() == 1 )
        {
            if( no_match_stranded_interior() )
                return true;
            else if( no_match_stranded_side() )
                return true;
            else if( no_match_mismatch_side() )
                return true;
        }*/

    }

    if( AreAllColorsUnmatchable() )
    {
        //LOG(("All colors unmatchable!\n"));
        return true;
    }
    if( DoCubesOnlyHaveStrandedDots() )
    {
        //LOG(("stranded dots!\n"));
        return true;
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

    //check if we have wilds on one cube and no chromits on any other
    unsigned int numCubesWithChromits = 0;

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].getNumDots() > 0 )
            numCubesWithChromits++;
    }

    return ( numCubesWithChromits == 1 );
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
        if( m_cubes[i].hasColor(color, true) )
        {
            total++;
            aHasColor[i] = true;
            //LOG(("cube %d has color %d\n", i, color));
        }
        else
            aHasColor[i] = false;
    }

    if( total <= 1 )
    {
        //LOG(("%d cubes have color %d\n", total, color));
        return true;
    }

    int numCorners = 0;
    bool side1 = false;
    bool side2 = false;

    //also, make sure these colors on these cubes can possibly match
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        //LOG(("cube %d do we have color? %d\n", i, aHasColor[i]));
        if( aHasColor[i] )
        {
            CubeWrapper::GridTestInfo testInfo = { color, false, false, false };

            LOG(("local testInfo at %x\n", &testInfo));

            m_cubes[i].UpdateColorPositions( testInfo );

            LOG(("cube %d has corners = %d, side1 = %d, side2 = %d", i, testInfo.bCorners, testInfo.bSide1, testInfo.bSide2));

            if( testInfo.bCorners )
            {
                numCorners++;
                //this color has corners on multiple cubes, there's a match!
                if( numCorners >= 2 )
                    return false;
            }

            //side1 on one cube can match side2 on another
            if( testInfo.bSide1 )
            {
                if( side2 )
                    return false;
            }
            if( testInfo.bSide2 )
            {
                if( side1 )
                    return false;
            }

            if( testInfo.bSide1 )
                side1 = true;
            if( testInfo.bSide2 )
                side2 = true;
        }
    }

    LOG(("Failed to find a match!  color %d\n", color));
    return true;
}

//return the number of cubes with the given color
unsigned int Game::NumCubesWithColor( unsigned int color ) const
{
    int total = 0;
    int totalWithWilds = 0;

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].hasColor(color, false) )
        {
            total++;
            totalWithWilds++;
        }
        else if( m_cubes[i].hasColor(color, true) )
        {
            totalWithWilds++;
        }
    }

    if( total == 0 )
        return 0;
    else
        return totalWithWilds;
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

	Int2 aBuddies[3];
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
    {
        if( m_mode == MODE_BLITZ )
            return s_HighScores[ index ];
        else
            return s_HighCubes[ index ];
    }
    else
        return 0;
}



void Game::enterScore()
{
    unsigned int *pScores;
    unsigned int score;

    if( m_mode == MODE_BLITZ )
    {
        pScores = s_HighScores;
        score = m_iScore;
    }
    else
    {
        pScores = s_HighCubes;
        score = getDisplayedLevel();
    }

    //walk backwards through the high score list and see which ones we can pick off
    for( int i = (int)NUM_HIGH_SCORES - 1; i >= 0; i-- )
    {
        if( pScores[i] < score )
        {
            if( i < (int)NUM_HIGH_SCORES - 1 )
            {
                pScores[i+1] = pScores[i];

                if( i == 0 )
                    pScores[0] = score;
            }
        }
        else
        {
            if( i < (int)NUM_HIGH_SCORES - 1 )
            {
                pScores[i+1] = score;
            }

            break;
        }
    }
}


void Game::playSound( const AssetAudio &sound )
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

const AssetAudio *SLOSH_SOUNDS[Game::NUM_SLOSH_SOUNDS] =
{
  &slosh_multi_01,
    &slosh_multi_02,
};


//play a random slosh sound
void Game::playSlosh()
{
    if( m_lastSloshTime.isValid() && SystemTime::now() - m_lastSloshTime > SLOSH_THRESHOLD )
    {
        int index = random.randrange( NUM_SLOSH_SOUNDS );
        playSound(*SLOSH_SOUNDS[index]);

        m_lastSloshTime = SystemTime::now();
    }
}



//destroy all dots of the given color
void Game::BlowAll( unsigned int color )
{    
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        //make sure no glimmers happen
        m_cubes[i].StopGlimmer();
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
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        m_cubes[i].TurnOffSprites();
    }

    enterScore();

    if( m_mode == MODE_SURVIVAL )
    {
        for( int i = 0; i < NUM_CUBES; i++ )
        {
            m_cubes[i].getBanner().SetMessage( "NO MORE MATCHES", 3.5f );
        }
    }

    setState( STATE_GAMEOVERBANNER );
    //TransitionToState( STATE_POSTGAME );

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

unsigned int Game::CountEmptyCubes() const
{
    int count = 0;

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[i].isEmpty() )
            count++;
    }

    return count;
}


//add one piece to the game
void Game::RespawnOnePiece()
{
    //cycle through our cubes
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( m_cubes[m_cubeToRespawn].isFull() )
        {
            m_cubeToRespawn++;
            if( m_cubeToRespawn >= NUM_CUBES )
                m_cubeToRespawn = 0;
        }
        else
        {
            m_cubes[m_cubeToRespawn].RespawnOnePiece();
            break;
        }
    }

    m_cubeToRespawn++;
    if( m_cubeToRespawn >= NUM_CUBES )
        m_cubeToRespawn = 0;

    m_fTimeTillRespawn = TIME_TO_RESPAWN;
}


void Game::UpCombo()
{
    if( m_mode == MODE_BLITZ )
    {
        if( m_fTimeSinceCombo > 0.0f )
        {
            if( m_fTimeSinceCombo > COMBO_TIME_THRESHOLD )
                m_comboCount = 0;

            m_comboCount++;
            m_fTimeSinceCombo = 0.0f;
        }
    }
}


void Game::UpMultiplier()
{
    ASSERT( m_mode == MODE_BLITZ );
    ASSERT( m_Multiplier < MAX_MULTIPLIER - 1 );

    m_Multiplier++;

    //find all existing multpilier dots and tick them up one
    for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].UpMultiplier();
}


void Game::check_puzzle()
{
    for( int i = 0; i < NUM_CUBES; i++ )
    {
        if( !m_cubes[i].isEmpty() )
        {
            //did we lose?
            if( NoMatches() )
                TransitionToState( STATE_FAILPUZZLE );
                //gotoNextPuzzle( false );
            return;
        }
    }

    //win!
    TransitionToState( STATE_GOODJOB );
}


const Puzzle *Game::GetPuzzle()
{
    return Puzzle::GetPuzzle( m_iLevel );
}

const PuzzleCubeData *Game::GetPuzzleData( unsigned int id )
{
    const Puzzle *pPuzzle = GetPuzzle();

    if( !pPuzzle )
        return NULL;

    //TODO, make this work with sparse ids
    return pPuzzle->getCubeData( id );
}


void Game::gotoNextPuzzle( bool bAdvance )
{
    if( bAdvance )
        m_iLevel++;

    //TODO, check if all puzzles were completed
    const Puzzle *pPuzzle = Puzzle::GetPuzzle( m_iLevel );

    //TODO, end game celebration!
    if( !pPuzzle )
        return;

    //intro puzzles (<3 cubes) will jump straight into the next puzzle
    if( pPuzzle->m_numCubes < 3 )
        TransitionToState( STATE_INTRO );
    else
        TransitionToState( STATE_NEXTPUZZLE );

    for( int i = 0; i < NUM_CUBES; i++ )
        m_cubes[i].Refill();
}



bool Game::AreMovesLegal() const
{
    if( m_mode == MODE_BLITZ )
    {
        if( getState() == STATE_INTRO )
            return true;
        if( m_timer.getTime() < 0.0f )
            return false;
    }

    return getState() == STATE_PLAYING;
}



void Game::ReturnToMainMenu()
{
    Reset( false );
}



void Game::HandleMenu()
{
    const unsigned int MAX_MENU_ITEMS = 10;

    MenuItem allmenuitems[][ MAX_MENU_ITEMS ] =
    {
        //main menu
        { {&UI_Main_Menu_Survival, NULL}, {&UI_Main_Menu_Blitz, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {NULL, NULL} },
        //puzzle menu
        { {&UI_Main_Menu_Continue, NULL}, {&UI_Main_Menu_NewGame, NULL}, {&UI_Main_Menu_ChapterSelect, NULL}, {&UI_Main_Menu_Back, NULL}, {NULL, NULL} },
        //chapter select menu
        { {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_Chapter, NULL}, {&UI_Main_Menu_PuzzleBack, NULL}, {&UI_Main_Menu_PuzzleBack, NULL}, {NULL, NULL} },
        //puzzle select menu
        { {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_Puzzle, NULL}, {&UI_Main_Menu_PuzzleBack, NULL}, {&UI_Main_Menu_PuzzleBack, NULL}, {NULL, NULL} },
    };

    MenuAssets allmenuassets[] =
    {
        //main menu
        {&White, &UI_Main_Menu_TipsTouch, &UI_Main_Menu_Topbar, {&UI_Main_Menu_TipsTouch, &UI_Main_Menu_TipsTilt, NULL}},
        //puzzle menu
        {&White, &UI_Main_Menu_TipsTouch, &UI_Puzzle_Menu_Topbar, {&UI_Main_Menu_TipsTouch, &UI_Main_Menu_TipsTilt, NULL}},
        //chapter select menu
        {&White, &UI_Main_Menu_TipsTouch, &UI_ChapterSelect_Topbar, {&UI_Main_Menu_TipsTouch, &UI_Main_Menu_TipsTilt, NULL}},
        //puzzle select menu
        {&White, &UI_Main_Menu_TipsTouch, &UI_PuzzleSelect_Menu_Topbar, {&UI_Main_Menu_TipsTouch, &UI_Main_Menu_TipsTilt, NULL}},
    };

    MenuItem *pItems = allmenuitems[0];
    MenuAssets *pAssets = &allmenuassets[0];

    unsigned int numSelectables = 0;
    unsigned int numTotal = 0;

    switch( m_state )
    {
        case STATE_MAINMENU:
        {
            pItems = allmenuitems[0];
            pAssets = &allmenuassets[0];
            break;
        }
        case STATE_PUZZLEMENU:
        {
            pItems = allmenuitems[1];
            pAssets = &allmenuassets[1];
            break;
        }
        case STATE_CHAPTERSELECTMENU:
        {
            pItems = allmenuitems[2];
            pAssets = &allmenuassets[2];

            numSelectables = Puzzle::GetChapter( m_iFurthestProgress ) + 1;

            //LOG(( "Num selectables = %d\n", numSelectables));

            MenuItem unlocked = {&UI_Main_Menu_Chapter, NULL};
            MenuItem locked = {&UI_Main_Menu_ChapterLock, NULL};

            numTotal = Puzzle::GetNumChapters();

            ASSERT( numTotal <= MAX_MENU_ITEMS - 2 );

            //certain menu types will have to reinit the menu items list
            for( int i = 0; i < numTotal; i++ )
            {
                if( i < numSelectables )
                    allmenuitems[2][i] = unlocked;
                else
                    allmenuitems[2][i] = locked;
            }

            MenuItem back = {&UI_Main_Menu_PuzzleBack, NULL};
            MenuItem nullItem = {NULL, NULL};
            allmenuitems[2][ numTotal ] = back;
            allmenuitems[2][ numTotal + 1 ] = nullItem;
            break;
        }
        case STATE_PUZZLESELECTMENU:
        {
            pItems = allmenuitems[3];
            pAssets = &allmenuassets[3];
            //certain menu types will have to reinit the menu items list

            MenuItem unlocked = {&UI_Main_Menu_Puzzle, NULL};
            MenuItem locked = {&UI_Main_Menu_PuzzleLock, NULL};

            numTotal = Puzzle::GetNumPuzzlesInChapter( m_iChapterViewed );
            int puzzleOffset = Puzzle::GetPuzzleOffset( m_iChapterViewed );

            //certain menu types will have to reinit the menu items list
            for( int i = 0; i < numTotal; i++ )
            {
                if( i + puzzleOffset <= m_iFurthestProgress )
                {
                    allmenuitems[3][i] = unlocked;
                    numSelectables++;
                }
                else
                    allmenuitems[3][i] = locked;
            }

            MenuItem back = {&UI_Main_Menu_PuzzleBack, NULL};
            MenuItem nullItem = {NULL, NULL};
            allmenuitems[3][ numTotal ] = back;
            allmenuitems[3][ numTotal + 1 ] = nullItem;
            break;
        }
        default:
        ASSERT(0);
    }

    struct MenuEvent e;
    Menu menu(&m_cubes[0].GetCube(), pAssets, pItems);

    menu.setIconYOffset( 25 );

    //clear out the other cubes
    for( int i = 1; i < NUM_CUBES; i++ )
    {
        m_cubes[i].GetVid().clear( GemEmpty.tiles[0] );
    }

    while(menu.pollEvent(&e))
    {
        switch(e.type)
        {
            case MENU_ITEM_PRESS:
            {
                switch( m_state )
                {
                    case STATE_CHAPTERSELECTMENU:
                    case STATE_PUZZLESELECTMENU:
                    {
                        //LOG(( "item %d, num selectables = %d, num Total = %d\n", e.item, numSelectables, numTotal ));
                        if( e.item < numTotal && e.item >= numSelectables )
                            menu.preventDefault();
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case MENU_ITEM_ARRIVE:
            {
                switch( m_state )
                {
                    case STATE_PUZZLEMENU:
                    {
                        if( e.item == 0 )
                        {
                            int progress = m_iFurthestProgress + 1;
                            DrawSpriteNum( m_cubes[0].GetVid(), progress, Vec2( 64, 64 ) );
                        }
                        break;
                    }
                    case STATE_CHAPTERSELECTMENU:
                    {
                        if( e.item < numTotal )
                        {
                            DrawSpriteNum( m_cubes[0].GetVid(), e.item + 1, Vec2( 42, 50 ) );
                        }
                        break;
                    }
                    case STATE_PUZZLESELECTMENU:
                    {
                        if( e.item < numTotal )
                        {
                            int puzzleNum = e.item + Puzzle::GetPuzzleOffset( m_iChapterViewed ) + 1;
                            DrawSpriteNum( m_cubes[0].GetVid(), puzzleNum, Vec2( 52, 49 ) );
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case MENU_ITEM_DEPART:
            {
                m_cubes[0].GetVid().resizeSprite( 0, 0, 0 );
                m_cubes[0].GetVid().resizeSprite( 1, 0, 0 );
                break;
            }
            default:
                break;
        }
    }


    switch( m_state )
    {
        case STATE_MAINMENU:
        {
            GameState targetState = STATE_INTRO;
            m_mode = (GameMode)e.item;
            m_iLevel = 0;

            if( m_mode == MODE_BLITZ )
                m_iLevel = 3;
            else if( m_mode == MODE_PUZZLE && m_iFurthestProgress > 0 )
                targetState = STATE_PUZZLEMENU;

            TransitionToState( targetState );
            break;
        }
        case STATE_PUZZLEMENU:
        {
            switch( e.item )
            {
                //continue
                case 0:
                {
                    m_iLevel = m_iFurthestProgress;
                    gotoNextPuzzle( false );
                    break;
                }
                //new game
                case 1:
                {
                    m_iLevel = 0;
                    TransitionToState( STATE_INTRO );
                    break;
                }
                //chapter select
                case 2:
                {
                    TransitionToState( STATE_CHAPTERSELECTMENU );
                    break;
                }
                //back
                case 3:
                {
                    TransitionToState( STATE_MAINMENU );
                    break;
                }
            }

            break;
        }
        case STATE_CHAPTERSELECTMENU:
        {
            if( e.item < numSelectables )
            {
                m_iChapterViewed = e.item;
                TransitionToState( STATE_PUZZLESELECTMENU );
            }
            else if( e.item == numTotal )
                TransitionToState( STATE_PUZZLEMENU );
            break;
        }
        case STATE_PUZZLESELECTMENU:
        {
            if( e.item < numSelectables )
            {
                m_iLevel = e.item + Puzzle::GetPuzzleOffset( m_iChapterViewed );
                gotoNextPuzzle( false );
            }
            else if( e.item == numTotal )
                TransitionToState( STATE_CHAPTERSELECTMENU );
            break;
        }
        default:
            ASSERT(0);
    }

    for( int i = 0; i < NUM_CUBES; i++ )
    {
        m_cubes[i].Reset();
    }
}
