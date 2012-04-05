/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cubewrapper.h"
#include "game.h"
#include "assets.gen.h"
#include "utils.h"
#include "string.h"
#include "config.h"
#include "puzzle.h"

static _SYSCubeID s_id = CUBE_ID_BASE;

//const float CubeWrapper::SHAKE_FILL_DELAY = 1.0f;
const float CubeWrapper::SHAKE_FILL_DELAY = 0.4f;
const float CubeWrapper::SPRING_K_CONSTANT = 0.7f;
const float CubeWrapper::SPRING_DAMPENING_CONSTANT = 0.07f;
const float CubeWrapper::MOVEMENT_THRESHOLD = 4.7f;
//const float CubeWrapper::IDLE_TIME_THRESHOLD = 3.0f;
//const float CubeWrapper::IDLE_FINISH_THRESHOLD = IDLE_TIME_THRESHOLD + ( GridSlot::NUM_IDLE_FRAMES * GridSlot::NUM_FRAMES_PER_IDLE_ANIM_FRAME * 1 / 60.0f );
const float CubeWrapper::MIN_GLIMMER_TIME = 20.0f;
const float CubeWrapper::MAX_GLIMMER_TIME = 30.0f;
const float CubeWrapper::TILT_SOUND_EPSILON = 5.0f;
//const float CubeWrapper::SHOW_BONUS_TIME = 3.1f;
const float CubeWrapper::TOUCH_TIME_FOR_MENU = 1.7f;
//how long we wait until we autorefill an empty cube in survival mode
const float CubeWrapper::AUTOREFILL_TIME = 3.5f;

CubeWrapper::CubeWrapper() : m_cube(s_id++), m_vid(m_cube.vbuf), m_rom(m_cube.vbuf),
        m_bg1helper( m_cube ), m_state( STATE_PLAYING ),
        m_fTouchTime( 0.0f ), m_curFluidDir(Vec2( 0, 0 )), m_curFluidVel(Vec2( 0, 0 )), m_stateTime( 0.0f ),
        m_lastTiltDir( 0 ), m_numQueuedClears( 0 ), m_queuedFlush( false ), m_dirty( true ),
        m_bubbles( m_vid )
{
	for( int i = 0; i < NUM_SIDES; i++ )
	{
		m_neighbors[i] = -1;
	}

	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			slot.Init( this, i, j );
		}
	}
}


void CubeWrapper::Init( AssetGroup &assets )
{
    m_cube.enable();
#if LOAD_ASSETS
    m_cube.loadAssets( assets );
#endif

    m_rom.init();
    m_rom.BG0_text(Vec2(1,1), "Loading...");
}


void CubeWrapper::Reset()
{
    m_ShakeTime = SystemTime();
    setState( STATE_PLAYING );
    m_idleTimer = 0.0f;

    //clear out dots
    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            GridSlot &slot = m_grid[i][j];
            slot.Init( this, i, j );
        }
    }

    m_timeTillGlimmer = 0.0f;
    m_bg1helper.Clear();
    m_queuedFlush = true;
    m_intro.Reset();
    m_glimmer.Reset();
    m_bubbles.Reset( m_vid );
    m_numQueuedClears = 0;

    m_dirty = true;
	Refill();
}

bool CubeWrapper::DrawProgress( AssetGroup &assets )
{
    m_rom.BG0_progressBar(Vec2(0,7), m_cube.assetProgress(assets, m_vid.LCD_width), 2);
        
	return m_cube.assetDone(assets);
}

void CubeWrapper::Draw()
{   
	switch( Game::Inst().getState() )
	{
		case Game::STATE_SPLASH:
		{
            //m_vid.BG0_drawAsset(Vec2(0,0), Cover, 0);
			break;
		}
        case Game::STATE_INTRO:
        {
            m_intro.Draw( Game::Inst().getTimer(), m_bg1helper, m_vid, this );

            //possible to be already playing in intro mode
            if( m_intro.getState() != Intro::STATE_BALLEXPLOSION )
                DrawGrid();

            m_queuedFlush = true;
            break;
        }
		case Game::STATE_PLAYING:
		{
			switch( m_state )
			{
				case STATE_PLAYING:
				{
                    //special case - this cube shows instructions
                    if( Game::Inst().getMode() == Game::MODE_PUZZLE)
                    {
                        const Puzzle *pPuzzle = Game::Inst().GetPuzzle();
                        if( Game::Inst().getWrapperIndex( this ) == 2 && pPuzzle->m_numCubes < 3 )
                        {
                            DrawMessageBoxWithText( pPuzzle->m_pInstr );
                            break;
                        }

                        if( isEmpty() )
                        {
                            m_vid.BG0_drawAsset(Vec2(0,0), Lumes_Neutral, 0);
                            TurnOffSprites();
                            break;
                        }
                    }

                    DrawGrid();

                    //draw glimmer before timer
                    m_glimmer.Draw( m_bg1helper, this );

                    if( Game::Inst().getMode() == Game::MODE_BLITZ )
                    {
                        Game::Inst().getTimer().Draw( m_bg1helper, m_vid );
                    }
                    if( m_banner.IsActive() )
                        m_banner.Draw( m_bg1helper );

                    //rocks
                    if( Game::Inst().getMode() != Game::MODE_BLITZ )
                    {
                        for( int i = 0; i < RockExplosion::MAX_ROCK_EXPLOSIONS; i++ )
                            m_aExplosions[i].Draw( m_vid, i );
                    }

                    m_bubbles.Draw( m_vid, this );
                    m_floatscore.Draw( m_bg1helper );

                    m_queuedFlush = true;

                    //super debug code!
                    //Banner::DrawScore( m_bg1helper, Vec2( 0, 0 ), Banner::LEFT, m_cube.id() );

                    //for debugging combo count
                    //if( Game::Inst().getMode() == Game::MODE_BLITZ )
                      //  Banner::DrawScore( m_bg1helper, Vec2( 0, 0 ), Banner::LEFT, Game::Inst().GetComboCount() );

					break;
				}
				case STATE_EMPTY:
				{
                    m_vid.BG0_drawAsset(Vec2(0,0), UI_NCubesCleared, 0);
                    int level = Game::Inst().getDisplayedLevel();

                    Banner::DrawScore( m_bg1helper, Vec2<int>( Banner::CENTER_PT, 3 ),
                                       Banner::CENTER, level );

                    m_vid.BG1_setPanning( Vec2( 0, -4 ) );

                    m_queuedFlush = true;
					break;
				}
                case STATE_REFILL:
                {
                    m_intro.Draw( Game::Inst().getTimer(), m_bg1helper, m_vid, this );
                    m_queuedFlush = true;
                    break;
                }
			}			
			break;
		}
		case Game::STATE_POSTGAME:
		{
            if( !m_dirty )
            {
                //force touch of a cube, maybe it'll fix things
                //m_cube.vbuf.touch();
                break;
            }

            TurnOffSprites();

            //m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);

            if( Game::Inst().getWrapperIndex( this ) == 0 )
            {
                int highScoreIndex = -1;
                unsigned int myScore = Game::Inst().getScore();

                for( unsigned int i = 0; i < Game::NUM_HIGH_SCORES; i++ )
                {
                    if( myScore == Game::Inst().getHighScore(i) )
                    {
                        highScoreIndex = i;
                        break;
                    }
                }

                if( highScoreIndex >= 0 )
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), UI_Highscores, 0);
                    m_vid.BG0_drawAsset(Vec2(0,HIGH_SCORE_OFFSET+2*highScoreIndex), UI_Highlight, 0);
                }
                else
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), UI_Highscores_lowscore, 0);
                    Banner::DrawScore( m_bg1helper, Vec2( 11, 14 ), Banner::CENTER, myScore );
                }

                for( unsigned int i = 0; i < Game::NUM_HIGH_SCORES; i++ )
                {
                    int score = Game::Inst().getHighScore(i);

                    Banner::DrawScore( m_bg1helper, Vec2<int>( 7, HIGH_SCORE_OFFSET+2*i ), Banner::RIGHT, score );

                    if( i == Game::NUM_HIGH_SCORES - 2 && highScoreIndex < 0 )
                        break;
                }
            }
            else if( Game::Inst().getWrapperIndex( this ) == 1 )
            {
                m_vid.BG0_drawAsset(Vec2(0,0), UI_ExitGame, 0);
            }
            else if( Game::Inst().getWrapperIndex( this ) == 2 )
            {
                m_vid.BG0_drawAsset(Vec2(0,0), UI_Touch_Replay, 0);
                //m_bg1helper.DrawTextf( Vec2( 4, 3 ), Font, "Shake or\nNeighbor\nfor new\n game" );
            }

            m_queuedFlush = true;
            m_dirty = false;

			break;
		}
        case Game::STATE_GOODJOB:
        {
            TurnOffSprites();

            if( Game::Inst().getStateTime() < Game::LUMES_FACE_TIME )
            {
                m_bg1helper.Clear();
                m_bg1helper.Flush();
                m_vid.BG0_drawAsset(Vec2(0,0), Lumes_Happy, 0);
            }
            else
                DrawMessageBoxWithText( "Good Job" );
            break;
        }
        case Game::STATE_FAILPUZZLE:
        {
            TurnOffSprites();

            if( Game::Inst().getStateTime() < Game::LUMES_FACE_TIME )
                m_vid.BG0_drawAsset(Vec2(0,0), Lumes_Sad, 0);
            else
            {
                switch( Game::Inst().getWrapperIndex( this ) )
                {
                    case 0:
                    {
                        DrawMessageBoxWithText( "Oops.  Try again!" );
                        break;
                    }
                    case 1:
                    {
                        m_vid.BG0_drawAsset(Vec2(0,0), UI_ExitGame, 0);
                        break;
                    }
                    case 2:
                    {
                        m_vid.BG0_drawAsset(Vec2(0,0), UI_Game_Menu_Restart, 0);
                        break;
                    }
                    default:
                        m_vid.clear( GemEmpty.tiles[0] );
                        break;
                }
            }
            break;
        }
        case Game::STATE_NEXTPUZZLE:
        {
            if( Game::Inst().getWrapperIndex( this ) == 0 )
            {
                const Puzzle *pPuzzle = Game::Inst().GetPuzzle();

                if( pPuzzle )
                {
                    String<64> buf;
                    buf << "Puzzle: " << pPuzzle->m_pName << " (" << Game::Inst().GetPuzzleIndex() << "/" << Puzzle::GetNumPuzzles() << ")";
                    DrawMessageBoxWithText( buf );
                }
            }
            else if( Game::Inst().getWrapperIndex( this ) == 1 )
            {
                const Puzzle *pPuzzle = Game::Inst().GetPuzzle();

                if( pPuzzle )
                {
                    DrawMessageBoxWithText( pPuzzle->m_pInstr );
                }
            }
            else if( Game::Inst().getWrapperIndex( this ) == 2 )
            {
                m_bg1helper.Clear();
                m_bg1helper.Flush();
                m_vid.BG0_drawAsset(Vec2(0,0), UI_Game_Menu_Continue, 0);
            }
            break;
        }
        case Game::STATE_GAMEMENU:
        {
            TurnOffSprites();
            m_bg1helper.Clear();
            m_bg1helper.Flush();

            switch( Game::Inst().getWrapperIndex( this ) )
            {
                case 0:
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), UI_Game_Menu_Restart, 0);
                    break;
                }
                case 1:
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), UI_ExitGame, 0);
                    break;
                }
                case 2:
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), UI_Game_Menu_Continue, 0);
                    break;
                }
                default:
                    m_vid.clear( GemEmpty.tiles[0] );
                    break;
            }

            break;
        }
        case Game::STATE_GAMEOVERBANNER:
        {
            m_banner.Draw( m_bg1helper );
            m_queuedFlush = true;
            break;
        }
		default:
			break;
	}

    m_numQueuedClears = 0;

    //TODO, fix hack!
    m_cube.vbuf.touch();
}


void CubeWrapper::Update(SystemTime t, TimeDelta dt)
{
    m_stateTime += dt;

    if( Game::Inst().getState() == Game::STATE_PLAYING || Game::Inst().getState() == Game::STATE_INTRO )
    {
        for( Cube::Side i = 0; i < NUM_SIDES; i++ )
        {
            bool newValue = m_cube.hasPhysicalNeighborAt(i);
            Cube::ID id = m_cube.physicalNeighborAt(i);

            //newly neighbored
            if( newValue )
            {
                if( id != m_neighbors[i] )
                {
                    Game::Inst().setTestMatchFlag();
                    m_neighbors[i] = id - CUBE_ID_BASE;
                }
            }
            else
                m_neighbors[i] = -1;
        }
    }

    if( Game::Inst().getState() == Game::STATE_INTRO || m_state == STATE_REFILL )
    {
        //update all dots
        for( int i = 0; i < NUM_ROWS; i++ )
        {
            for( int j = 0; j < NUM_COLS; j++ )
            {
                GridSlot &slot = m_grid[i][j];
                slot.Update( t );
            }
        }

        if( !m_intro.Update( t, dt, m_banner ) )
        {
            if( m_state == STATE_REFILL )
                m_state = STATE_PLAYING;
        }
        return;
    }
    /*else if( m_state == STATE_CUBEBONUS )
    {
        if( m_stateTime > SHOW_BONUS_TIME )
            setState( STATE_EMPTY );
    }*/

    if( Game::Inst().getState() == Game::STATE_PLAYING )
    {
        m_timeTillGlimmer -= dt;

        if( m_timeTillGlimmer < 0.0f )
        {
            m_timeTillGlimmer = Game::random.uniform( MIN_GLIMMER_TIME, MAX_GLIMMER_TIME );
            m_glimmer.Reset();
        }
        m_glimmer.Update( dt );

        //check for shaking
        if( Game::Inst().getMode() == Game::MODE_BLITZ && ( m_ShakeTime.isValid() && t - m_ShakeTime > SHAKE_FILL_DELAY ) )
        {
            m_ShakeTime = SystemTime();
            checkRefill();
        }

        if( _SYS_isTouching( m_cube.id() ) )
        {

            if( m_state != STATE_EMPTY )
            {
                m_fTouchTime += dt;

                if( m_fTouchTime > TOUCH_TIME_FOR_MENU )
                {
                    Game::Inst().setState( Game::STATE_GAMEMENU );
                    m_fTouchTime = 0.0f;
                    return;
                }
            }
        }
        else
            m_fTouchTime = 0.0f;

        //update all dots
        for( int i = 0; i < NUM_ROWS; i++ )
        {
            for( int j = 0; j < NUM_COLS; j++ )
            {
                GridSlot &slot = m_grid[i][j];
                slot.Update( t );
            }
        }

        //update rocks
        if( Game::Inst().getMode() != Game::MODE_BLITZ )
        {
            for( int i = 0; i < RockExplosion::MAX_ROCK_EXPLOSIONS; i++ )
                m_aExplosions[i].Update();
        }

        m_banner.Update(t);
        m_bubbles.Update(dt, getTiltDir() );
        m_floatscore.Update( dt );

        //tilt state
        _SYSAccelState state = _SYS_getAccel(m_cube.id());

        //try spring to target
        Float2 delta = Vec2<float>( state.x, state.y ) - m_curFluidDir;

        //hooke's law
        Float2 force = SPRING_K_CONSTANT * delta - SPRING_DAMPENING_CONSTANT * m_curFluidVel;

        /*if( force.len2() < MOVEMENT_THRESHOLD )
        {
            m_idleTimer += dt;

            if( m_idleTimer > IDLE_FINISH_THRESHOLD )
            {
                m_idleTimer = 0.0f;
                //kick off force in a random direction
                //for now, just single direction
                //force = Float2( 100.0f, 0.0f );
            }
        }
        else
            m_idleTimer = 0.0f;*/

        //if sign of velocity changes, play a slosh
        Float2 oldvel = m_curFluidVel;

        m_curFluidVel += force;
        m_curFluidDir += m_curFluidVel * float(dt);

        if( m_curFluidVel.x > TILT_SOUND_EPSILON || m_curFluidVel.y > TILT_SOUND_EPSILON )
        {
            if( oldvel.x * m_curFluidVel.x < 0.0f || oldvel.y * m_curFluidVel.y < 0.0f )
                Game::Inst().playSlosh();
        }

        //autorefill after a certain time
        if( m_state == STATE_EMPTY && m_stateTime > AUTOREFILL_TIME )
            checkRefill();
    }
    else if( Game::Inst().getState() == Game::STATE_GAMEOVERBANNER )
    {
        m_banner.Update( t );
    }
    /*else if( Game::Inst().getState() == Game::STATE_POSTGAME )
    {
        if( _SYS_isTouching( m_cube.id() ) || ( m_ShakeTime.isValid() && t - m_ShakeTime > SHAKE_FILL_DELAY ) )
        {
            if( Game::Inst().getWrapperIndex( this ) == 1 )
                Game::Inst().ReturnToMainMenu();
            else if( Game::Inst().getWrapperIndex( this ) == 2 )
                Game::Inst().setTestMatchFlag();

            m_ShakeTime = SystemTime();
        }
    }
    else if( Game::Inst().getState() == Game::STATE_NEXTPUZZLE )
    {
        if( _SYS_isTouching( m_cube.id() ) || ( m_ShakeTime.isValid() && t - m_ShakeTime > SHAKE_FILL_DELAY ) )
        {
            Game::Inst().setState( Game::STATE_INTRO );
            m_ShakeTime = SystemTime();
        }
    }*/
}


void CubeWrapper::vidInit()
{
	m_vid.init();
}


void CubeWrapper::Tilt( int dir )
{
	bool bChanged = false;

    if( Game::Inst().getState() != Game::STATE_PLAYING || m_ShakeTime.isValid() )
		return;

    if( !Game::Inst().AreMovesLegal() )
        return;

	//hastily ported from the python
	switch( dir )
	{
		case 0:
		{
			for( int i = 0; i < NUM_COLS; i++ )
			{
				for( int j = 0; j < NUM_ROWS; j++ )
				{
					//start shifting it over
					for( int k = j - 1; k >= 0; k-- )
					{
                        //DEBUG_LOG(("src at (%d, %d), dest at (%d, %d)\n", k+1, i, k, i));
						if( TryMove( k+1, i, k, i ) )
							bChanged = true;
						else
							break;
					}
				}
			}
			break;
		}
		case 1:
		{
			for( int i = 0; i < NUM_ROWS; i++ )
			{
				for( int j = 0; j < NUM_COLS; j++ )					
				{
					//start shifting it over
					for( int k = j - 1; k >= 0; k-- )
					{
                        //DEBUG_LOG(("src at (%d, %d), dest at (%d, %d)\n", i, k+1, i, k));
						if( TryMove( i, k+1, i, k ) )
							bChanged = true;
						else
							break;
					}
				}
			}
			break;
		}
		case 2:
		{
			for( int i = 0; i < NUM_COLS; i++ )
			{
				for( int j = NUM_ROWS - 1; j >= 0; j-- )
				{
					//start shifting it over
					for( int k = j + 1; k < NUM_ROWS; k++ )
					{
                        //DEBUG_LOG(("src at (%d, %d), dest at (%d, %d)\n", k-1, i, k, i));
						if( TryMove( k-1, i, k, i ) )
							bChanged = true;
						else
							break;
					}
				}
			}
			break;
		}
		case 3:
		{
			for( int i = 0; i < NUM_ROWS; i++ )
			{
				for( int j = NUM_COLS - 1; j >= 0; j-- )
				{
					//start shifting it over
					for( int k = j + 1; k < NUM_COLS; k++ )
					{
                        //DEBUG_LOG(("src at (%d, %d), dest at (%d, %d)\n", i, k-1, i, k));
						if( TryMove( i, k-1, i, k ) )
							bChanged = true;
						else
							break;
					}
				}
			}
			break;
		}
	}        

    if( bChanged )
    {

        //change all pending movers to movers
        for( int i = 0; i < NUM_ROWS; i++ )
        {
            for( int j = 0; j < NUM_COLS; j++ )
            {
                GridSlot &slot = m_grid[i][j];
                slot.startPendingMove();
            }
        }


		Game::Inst().setTestMatchFlag();
    }

    m_lastTiltDir = dir;
}


bool CubeWrapper::FakeTilt( int dir, GridSlot grid[][NUM_COLS] )
{
    bool bChanged = false;

    //hastily ported from the python
    switch( dir )
    {
        case 0:
        {
            for( int i = 0; i < NUM_COLS; i++ )
            {
                for( int j = 0; j < NUM_ROWS; j++ )
                {
                    //start shifting it over
                    for( int k = j - 1; k >= 0; k-- )
                    {
                        if( FakeTryMove( k+1, i, k, i, grid ) )
                            bChanged = true;
                        else
                            break;
                    }
                }
            }
            break;
        }
        case 1:
        {
            for( int i = 0; i < NUM_ROWS; i++ )
            {
                for( int j = 0; j < NUM_COLS; j++ )
                {
                    //start shifting it over
                    for( int k = j - 1; k >= 0; k-- )
                    {
                        if( FakeTryMove( i, k+1, i, k, grid ) )
                            bChanged = true;
                        else
                            break;
                    }
                }
            }
            break;
        }
        case 2:
        {
            for( int i = 0; i < NUM_COLS; i++ )
            {
                for( int j = NUM_ROWS - 1; j >= 0; j-- )
                {
                    //start shifting it over
                    for( int k = j + 1; k < NUM_ROWS; k++ )
                    {
                        if( FakeTryMove( k-1, i, k, i, grid ) )
                            bChanged = true;
                        else
                            break;
                    }
                }
            }
            break;
        }
        case 3:
        {
            for( int i = 0; i < NUM_ROWS; i++ )
            {
                for( int j = NUM_COLS - 1; j >= 0; j-- )
                {
                    //start shifting it over
                    for( int k = j + 1; k < NUM_COLS; k++ )
                    {
                        if( FakeTryMove( i, k-1, i, k, grid ) )
                            bChanged = true;
                        else
                            break;
                    }
                }
            }
            break;
        }
    }

    return bChanged;
}


void CubeWrapper::Shake( bool bShaking )
{
	if( bShaking )
        m_ShakeTime = SystemTime::now();
	else
		m_ShakeTime = SystemTime();
}


void CubeWrapper::Touch()
{           
    switch( Game::Inst().getState() )
    {
        case Game::STATE_PLAYING:
        {
            if( Game::Inst().getMode() == Game::MODE_SURVIVAL && m_state == STATE_EMPTY )
                checkRefill();
            break;
        }
        case Game::STATE_POSTGAME:
        {
            if( Game::Inst().getWrapperIndex( this ) == 1 )
                Game::Inst().ReturnToMainMenu();
            else if( Game::Inst().getWrapperIndex( this ) == 2 )
                Game::Inst().setTestMatchFlag();
            break;
        }
        case Game::STATE_NEXTPUZZLE:
        {
            Game::Inst().TransitionToState( Game::STATE_INTRO );
            break;
        }
        case Game::STATE_GAMEMENU:
        {
            if( Game::Inst().getWrapperIndex( this ) == 0 )
            {
                if( Game::Inst().getMode() == Game::MODE_PUZZLE )
                    Game::Inst().gotoNextPuzzle( false );
                else
                    Game::Inst().setTestMatchFlag();
            }
            else if( Game::Inst().getWrapperIndex( this ) == 1 )
                Game::Inst().ReturnToMainMenu();
            else if( Game::Inst().getWrapperIndex( this ) == 2 )
                Game::Inst().TransitionToState( Game::STATE_PLAYING );
            break;
        }
        case Game::STATE_FAILPUZZLE:
        {
            if( Game::Inst().getWrapperIndex( this ) == 1 )
                Game::Inst().ReturnToMainMenu();
            else if( Game::Inst().getWrapperIndex( this ) == 2 )
                Game::Inst().gotoNextPuzzle( false );
            break;
        }
        default:
            break;
    }
}


//try moving a gem from row1/col1 to row2/col2
//return if successful
bool CubeWrapper::TryMove( int row1, int col1, int row2, int col2 )
{
	//start shifting it over
	GridSlot &slot = m_grid[row1][col1];
	GridSlot &dest = m_grid[row2][col2];

    if( !dest.isEmpty() )
		return false;

    if( slot.isTiltable() )
	{
        if( slot.IsFixed() )
        {
            slot.setFixedAttempt();
            return false;
        }
        else
        {
            dest.TiltFrom(slot);
            slot.setEmpty();
            Game::Inst().Stabilize();
            return true;
        }
	}

	return false;
}


bool CubeWrapper::FakeTryMove( int row1, int col1, int row2, int col2, GridSlot grid[][NUM_COLS] )
{
    //start shifting it over
    GridSlot &slot = grid[row1][col1];
    GridSlot &dest = grid[row2][col2];

    if( !dest.isEmpty() )
        return false;

    if( slot.isTiltable() )
    {
        if( !slot.IsFixed() )
        {
            dest.TiltFrom(slot);
            slot.setEmpty();
            return true;
        }
    }

    return false;
}


//only test matches with neighbors with id less than ours.  This prevents double testing
void CubeWrapper::testMatches()
{
	for( int i = 0; i < NUM_SIDES; i++ )
	{
        if( m_neighbors[i] >= 0 && m_neighbors[i] < m_cube.id() - CUBE_ID_BASE )
		{
			//as long we we test one block going clockwise, and the other going counter-clockwise, we'll match up
            int side = Game::Inst().m_cubes[m_neighbors[i]].GetSideNeighboredOn( 0, m_cube );

			//no good, just set our flag again and return
			if( side < 0 )
			{
				Game::Inst().setTestMatchFlag();
				return;
			}

			//fill two 4 element pointer arrays of grid slots representing what we need to match up
			GridSlot *ourGems[4];
			GridSlot *theirGems[4];

			FillSlotArray( ourGems, i, true );

            CubeWrapper &otherCube = Game::Inst().m_cubes[m_neighbors[i]];
			otherCube.FillSlotArray( theirGems, side, false );

			//compare the two
			for( int j = 0; j < NUM_ROWS; j++ )
			{
                if( ourGems[j]->isMatchable() && theirGems[j]->isMatchable() )
				{
                    bool bMatched = true;

                    //hypercolor madness
                    if( ourGems[j]->getColor() == GridSlot::HYPERCOLOR )
                    {
                        Game::Inst().BlowAll( theirGems[j]->getColor() );
                        ourGems[j]->mark();
                    }
                    else if( theirGems[j]->getColor() == GridSlot::HYPERCOLOR )
                    {
                        Game::Inst().BlowAll( ourGems[j]->getColor() );
                        theirGems[j]->mark();
                    }
                    //rocks can't match
                    else if( ourGems[j]->getColor() == GridSlot::ROCKCOLOR )
                    {
                        bMatched = false;
                    }
                    else if( ourGems[j]->getColor() == GridSlot::RAINBALLCOLOR )
                    {
                        ourGems[j]->RainballMorph( theirGems[j]->getColor() );
                        ourGems[j]->mark();
                        theirGems[j]->mark();
                    }
                    else if( theirGems[j]->getColor() == GridSlot::RAINBALLCOLOR )
                    {
                        theirGems[j]->RainballMorph( ourGems[j]->getColor() );
                        ourGems[j]->mark();
                        theirGems[j]->mark();
                    }
                    else if( ourGems[j]->getColor() == theirGems[j]->getColor() )
                    {
                        ourGems[j]->mark();
                        theirGems[j]->mark();
                    }
                    else
                    {
                        bMatched = false;
                    }

                    if( bMatched )
                        Game::Inst().UpCombo();
				}
			}
        }
	}
}


void CubeWrapper::FillSlotArray( GridSlot **gems, int side, bool clockwise )
{
	switch( side )
	{
		case 0:
		{
			if( clockwise )
			{
				for( int i = 0; i < NUM_COLS; i++ )
					gems[i] = &m_grid[0][i];
			}
			else
			{
				for( int i = 0; i < NUM_COLS; i++ )
					gems[NUM_COLS - i - 1] = &m_grid[0][i];
			}
			break;
		}
		case 1:
		{
			if( clockwise )
			{
				for( int i = 0; i < NUM_ROWS; i++ )
					gems[NUM_ROWS - i - 1] = &m_grid[i][0];
			}
			else
			{
				for( int i = 0; i < NUM_ROWS; i++ )
					gems[i] = &m_grid[i][0];
			}
			break;
		}
		case 2:
		{
			if( clockwise )
			{
				for( int i = 0; i < NUM_COLS; i++ )
					gems[NUM_COLS - i - 1] = &m_grid[NUM_ROWS - 1][i];
			}
			else
			{
				for( int i = 0; i < NUM_COLS; i++ )
					gems[i] = &m_grid[NUM_ROWS - 1][i];
			}
			break;
		}
		case 3:
		{
			if( clockwise )
			{
				for( int i = 0; i < NUM_ROWS; i++ )
					gems[i] = &m_grid[i][NUM_COLS - 1];
			}
			else
			{
				for( int i = 0; i < NUM_ROWS; i++ )
					gems[NUM_ROWS - i - 1] = &m_grid[i][NUM_COLS - 1];
			}
			break;
		}
		default:
			return;
	}

	//debug
	PRINT( "side %d, clockwise %d\n", side, clockwise );
	for( int i = 0; i < NUM_ROWS; i++ )
		PRINT( "gem %d: Alive %d, color %d\n", i, gems[i]->isAlive(), gems[i]->getColor() );
}


int CubeWrapper::GetSideNeighboredOn( _SYSCubeID id, Cube &cube )
{
	for( int i = 0; i < NUM_SIDES; i++ )
	{
        if( m_neighbors[i] == cube.id() - CUBE_ID_BASE )
			return i;
	}

	return -1;
}


GridSlot *CubeWrapper::GetSlot( int row, int col )
{
	//PRINT( "trying to retrieve %d, %d", row, col );
	if( row >= 0 && row < NUM_ROWS && col >= 0 && col < NUM_COLS )
	{
		return &(m_grid[row][col]);
	}

	return NULL;
}


bool CubeWrapper::isFull() const
{
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
            const GridSlot &slot = m_grid[i][j];
			if( slot.isEmpty() )
				return false;
		}
	}

	return true;
}

bool CubeWrapper::isEmpty() const
{
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
            const GridSlot &slot = m_grid[i][j];
			if( !slot.isEmpty() )
				return false;
		}
	}

	return true;
}


void CubeWrapper::checkRefill()
{
	if( isFull() )
            setState( STATE_PLAYING );
    else if( Game::Inst().getMode() == Game::MODE_PUZZLE )
	{
        //TODO - Smiley face replacement
        /*if( isEmpty() )
        {
            if( m_state != STATE_MESSAGING )
                setState( STATE_MESSAGING );
        }
        else
            setState( STATE_PLAYING );*/
	}
	else if( isEmpty() )
	{
        setState( STATE_REFILL );
        m_intro.Reset( true );
        Refill();

        m_vid.BG1_setPanning( Vec2( 0, 0 ) );

        /*if( Game::Inst().getMode() == Game::MODE_SURVIVAL && Game::Inst().getScore() > 0 )
		{
			m_banner.SetMessage( "FREE SHAKE!" );
        }*/
	}
	else
	{
        if( Game::Inst().getMode() != Game::MODE_SURVIVAL )
		{
            setState( STATE_REFILL );
            m_intro.Reset( true );
            Refill();
		}
        /*else if( Game::Inst().getShakesLeft() > 0 )
		{
            setState( STATE_REFILL );
            m_intro.Reset( true );
            Refill( true );
            Game::Inst().useShake();

            if( Game::Inst().getShakesLeft() == 0 )
				m_banner.SetMessage( "NO SHAKES LEFT" );
            else if( Game::Inst().getShakesLeft() == 1 )
				m_banner.SetMessage( "1 SHAKE LEFT" );
			else
			{
                String<16> buf;
                buf << Game::Inst().getShakesLeft() << " SHAKES LEFT";
                m_banner.SetMessage( buf );
			}
		}
		else
		{
            m_banner.SetMessage( "NO SHAKES LEFT" );
        }*/
	}
}
 

//massively ugly, but wanted to stick to the python functionality 
void CubeWrapper::Refill()
{ 
    if( Game::Inst().getMode() == Game::MODE_PUZZLE )
    {
        fillPuzzleCube();
        return;
    }

    const Level &level = Game::Inst().getLevel();

    //Game::Inst().playSound(glom_delay);

	/*
    Fill all empty spots in the grid with new gems.

    A filled grid should have the following characteristics:
    - Colors are distributed evenly.
    - Gems of a color have some coherence.
    - No fixed gems without a free gem of the same color.
    */

    //build a list of values to be added.
	uint32_t aNumNeeded[GridSlot::NUM_COLORS];
	unsigned int iNumFixed = 0;

	_SYS_memset32( aNumNeeded, 0, GridSlot::NUM_COLORS );

	int numDots = NUM_ROWS * NUM_COLS;

    for( unsigned int i = 0; i < level.m_numColors; i++ )
	{
        aNumNeeded[i] = numDots / level.m_numColors;
        if( i < numDots % level.m_numColors )
			aNumNeeded[i]++;
	}

    Int2 aEmptyLocs[NUM_ROWS * NUM_COLS] __attribute__ ((aligned(64)));

	unsigned int numEmpties = 0;

    //strip out existing gem values.
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
			{
				aNumNeeded[slot.getColor()]--;
				if( slot.IsFixed() )
					iNumFixed++;
			}
			else
            {
                aEmptyLocs[numEmpties++] = Vec2( i, j );
            }
		}
	}


	//random order to get locations in 
	int aLocIndices[NUM_ROWS * NUM_COLS];
	
	for( unsigned int i = 0; i < NUM_ROWS * NUM_COLS; i++ )
	{
		if( i < numEmpties )
			aLocIndices[i] = i;
		else
			aLocIndices[i] = -1;
	}

	//randomize
	for( unsigned int i = 0; i < numEmpties; i++ )
	{
		unsigned int randIndex = Game::random.randrange( numEmpties );
		unsigned int temp = aLocIndices[randIndex];
		aLocIndices[randIndex] = aLocIndices[i];
		aLocIndices[i] = temp;
	}

    //spawn rocks
    for( unsigned int i = 0; i < level.m_numRocks; i++ )
    {
        int curX = aEmptyLocs[aLocIndices[numEmpties - 1]].x;
        int curY = aEmptyLocs[aLocIndices[numEmpties - 1]].y;
        GridSlot &slot = m_grid[curX][curY];

        ASSERT( !slot.isAlive() );
        if( slot.isAlive() )
            continue;

        slot.FillColor( GridSlot::ROCKCOLOR );
        numEmpties--;
    }

	unsigned int iCurColor = 0;

	for( unsigned int i = 0; i < numEmpties; i++ )
	{
		int curX = aEmptyLocs[aLocIndices[i]].x;
		int curY = aEmptyLocs[aLocIndices[i]].y;
		GridSlot &slot = m_grid[curX][curY];

        //ASSERT( !slot.isAlive() );
		if( slot.isAlive() )
        {
			continue;
        }

		while( aNumNeeded[iCurColor] <= 0 && iCurColor < GridSlot::NUM_COLORS )
			iCurColor++;

		if( iCurColor >= GridSlot::NUM_COLORS )
			break;

		slot.FillColor(iCurColor);
		aNumNeeded[iCurColor]--;

		//cohesively fill a neighbor
		for( int j = 0; j < DEFAULT_COHESION; j++ )
		{
			if( aNumNeeded[iCurColor] <= 0 )
				break;

			while( true )
			{
				//random neighbor side
				int side = Game::random.randrange( NUM_SIDES );
				int neighborX = curX, neighborY = curY;
				//up left down right
				//note that x is rows in this context
				switch( side )
				{
					case 0:
						neighborX--;
						break;
					case 1:
						neighborY--;
						break;
					case 2:
						neighborX++;
						break;
					case 3:
						neighborY++;
						break;
				}

				GridSlot *pNeighbor = GetSlot( neighborX, neighborY );

				if( !pNeighbor )
					continue;


				//appears that a neighbor could be chosen many times since it doesn't force it to find another neighbor, 
				//but that's how the python code worked, so we'll go with it
				if( !pNeighbor->isAlive() )
				{
					pNeighbor->FillColor(iCurColor);
					aNumNeeded[iCurColor]--;
				}

				break;
			}
		}
	}

    
	//fixed gems
    while( iNumFixed < level.m_numFixed )
	{
		int toFix = Game::random.randrange( numEmpties );
		GridSlot &fix = m_grid[aEmptyLocs[toFix].x][aEmptyLocs[toFix].y];
		bool bFound = false;

        if( !fix.IsFixed() && !fix.IsSpecial() )
		{
			//make sure there's an another unfixed dot of this color
			for( int i = 0; i < NUM_ROWS; i++ )
			{
				for( int j = 0; j < NUM_COLS; j++ )
				{
					GridSlot &toCheck = m_grid[i][j];

					if( &toCheck != &fix && toCheck.getColor() == fix.getColor() && !toCheck.IsFixed() )
					{
						fix.MakeFixed();
						iNumFixed++;
						bFound = true;
						break;
					}
				}

				if( bFound )
					break;
			}
		}
	}    
}



//add one piece
void CubeWrapper::RespawnOnePiece()
{
    //grab a random empty location
    unsigned int numEmpties = 0;
    Int2 aEmptyLocs[NUM_ROWS * NUM_COLS];

    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            GridSlot &slot = m_grid[i][j];
            if( !slot.isAlive() )
                aEmptyLocs[numEmpties++] = Vec2( i, j );
        }
    }

    ASSERT( numEmpties > 0 );

    int toSpawn = Game::random.randrange( numEmpties );

    GridSlot &slot = m_grid[aEmptyLocs[toSpawn].x][aEmptyLocs[toSpawn].y];
    slot.FillColor( Game::random.randrange( Game::Inst().getLevel().m_numColors ), true );

    Game::Inst().playSound(bubble_pop_02);
}




//get the number of dots that are marked or exploding
unsigned int CubeWrapper::getNumMarked() const
{
	unsigned int numMarked = 0;

	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
			if( slot.isMarked() )
				numMarked++;
		}
	}

	return numMarked;
}


//fill in which colors we're using
void CubeWrapper::fillColorMap( bool *pMap ) const
{
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
				pMap[ slot.getColor() ] = true;
		}
	}
}


//do we have the given color anywhere?
bool CubeWrapper::hasColor( unsigned int color ) const
{
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
            if( slot.matchesColor( color ) )
				return true;
		}
	}

	return false;
}


//do we have stranded fixed dots?
bool CubeWrapper::hasStrandedFixedDots() const
{
	bool bHasFixed = false;
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
			{
				if( slot.IsFixed() )
				{
					if( i == 0 || i == NUM_ROWS - 1 || j == 0 || j == NUM_ROWS - 1 )
						return false;

					bHasFixed = true;
				}
                //we have floating non-rock dots
                else if( slot.getColor() != GridSlot::ROCKCOLOR )
					return false;
			}
		}
	}

	return bHasFixed;
}



bool CubeWrapper::allFixedDotsAreStrandedSide() const
{
	bool bHasFixed = false;
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
			{
				if( slot.IsFixed() )
				{
					//only works with 4 rows/cols
					if( ( i == 1 || i == 2 ) && ( j == 0 || j == 3 ) )
					{}
					else if( ( j == 1 || j == 2 ) && ( i == 0 || i == 3 ) )
					{}
					else
						return false;

					bHasFixed = true;
				}
                //we have floating non-rock dots
                else if( slot.getColor() != GridSlot::ROCKCOLOR )
					return false;
			}
		}
	}

	return bHasFixed;
}



bool CubeWrapper::hasNonStrandedDot() const
{
    if( hasStrandedFixedDots() )
        return false;

    if( allFixedDotsAreStrandedSide() && Game::Inst().OnlyOneOtherCorner( this ) )
        return false;

    //if we have a fixed dot, and all of the same color are only fixed dots, we need a reverse polarity fixed dot
    //if( AllFixedDotsAreUnMatchable() )
      //  return false;

    return true;
}


unsigned int CubeWrapper::getNumDots() const
{
	int count = 0;

	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
            const GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
				count++;
		}
	}

	return count;
}


unsigned int CubeWrapper::getNumCornerDots() const
{
	int count = 0;

	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
			{
				if( ( i == 0 || i == 3 ) && ( j == 0 || j == 3 ) )
					count++;
			}
		}
	}

	return count;
}


//returns if we have one and only one fixed dot (and zero floating dots)
//fills in the position of that dot
bool CubeWrapper::getFixedDot( Int2 &pos ) const
{
	int count = 0;

	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			const GridSlot &slot = m_grid[i][j];
			if( slot.isAlive() )
			{
				if( slot.IsFixed() )
				{
					count++;
					pos.x = i;
					pos.y = j;
				}
                else if( slot.getColor() != GridSlot::ROCKCOLOR )
					return false;
			}
		}
	}

	return count==1;
}


void CubeWrapper::checkEmpty()
{
    if( Game::Inst().getMode() == Game::MODE_SURVIVAL && isEmpty() && m_state != STATE_EMPTY )
    {
        Game::Inst().addLevel();
        m_bubbles.Reset( m_vid );
        setState( STATE_EMPTY );
    }
}


/*bool CubeWrapper::IsIdle() const
{
    return ( m_idleTimer > IDLE_TIME_THRESHOLD );
}
*/


void CubeWrapper::setState( CubeState state )
{
    m_state = state;
    m_stateTime = 0.0f;
}


//if we need to, flush bg1
void CubeWrapper::FlushBG1()
{
    if( m_queuedFlush )
    {
        m_bg1helper.Flush();
        m_queuedFlush = false;
    }
}


void CubeWrapper::QueueClear( Int2 &pos )
{
    m_queuedClears[m_numQueuedClears] = pos;
    m_numQueuedClears++;
    ASSERT( m_numQueuedClears <= NUM_ROWS * NUM_COLS );
}


//look for an empty spot to put a hyper dot
void CubeWrapper::SpawnSpecial( unsigned int color )
{
    unsigned int numEmpties = 0;
    Int2 aEmptyLocs[NUM_ROWS * NUM_COLS];

    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            GridSlot &slot = m_grid[i][j];

            if( slot.isEmpty() )
            {
                aEmptyLocs[numEmpties++] = Vec2( i, j );
            }
        }
    }

    unsigned int randIndex = Game::random.randrange( numEmpties );

    GridSlot &slot = m_grid[ aEmptyLocs[randIndex].x ][ aEmptyLocs[randIndex].y ];
    slot.FillColor( color );
}


bool CubeWrapper::SpawnMultiplier( unsigned int mult )
{
    //if we already have a multiplier, try a different cube
    for( int i = 1; i < NUM_ROWS - 1; i++ )
    {
        for( int j = 1; j < NUM_COLS - 1; j++ )
        {
            GridSlot &slot = m_grid[i][j];

            if( slot.isAlive() && slot.IsFixed() && slot.getMultiplier() > 1 )
                return false;
        }
    }

    //search the central locations for a fixed dot
    for( int i = 1; i < NUM_ROWS - 1; i++ )
    {
        for( int j = 1; j < NUM_COLS - 1; j++ )
        {
            GridSlot &slot = m_grid[i][j];

            if( slot.isAlive() && slot.IsFixed() )
            {
                slot.setMultiplier( mult );
                return true;
            }
        }
    }

    //no fixed dot, either spawn one or convert a regular dot into one
    int i = Game::random.random() > 0.5f ? 2 : 1;
    int j = Game::random.random() > 0.5f ? 2 : 1;

    GridSlot &slot = m_grid[i][j];

    if( !slot.isAlive() )
        slot.FillColor( Game::random.randrange( Game::Inst().getLevel().m_numColors ) );

    slot.MakeFixed();
    slot.setMultiplier( mult );

    return true;
}


//destroy all dots of the given color
void CubeWrapper::BlowAll( unsigned int color )
{
    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            GridSlot &slot = m_grid[i][j];

            if( slot.isMatchable() && slot.getColor() == color )
            {
                slot.mark();
                slot.Infect();
            }
        }
    }
}


bool CubeWrapper::HasHyperDot() const
{
    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            const GridSlot &slot = m_grid[i][j];

            if( slot.isAlive() && slot.getColor() == GridSlot::HYPERCOLOR )
                return true;
        }
    }

    return false;
}


//pretend to tilt this cube in a series of tilts, and update whether we see the given color on corners or side patterns 1 or 2
void CubeWrapper::UpdateColorPositions( unsigned int color, bool &bCorners, bool &side1, bool &side2 ) const
{
    TestGridForColor( m_grid, color, bCorners, side1, side2 );

    //we've already satisfied everything
    if( bCorners && side1 && side2 )
        return;

    if( !isFull() && HasFloatingDots() )
    {
        GridSlot grid[NUM_ROWS][NUM_COLS];

        //suddenly having problems with this memcpy
        //_SYS_memcpy8( (uint8_t *)grid, (uint8_t *)m_grid, sizeof( grid ) );
        for( int j = 0; j < NUM_ROWS; j++ )
        {
            for( int k = 0; k < NUM_COLS; k++ )
            {
                grid[j][k] = m_grid[j][k];
            }
        }

        //recursive function to tilt and test grid
        TiltAndTestGrid( grid, color, bCorners, side1, side2, TEST_TILT_ITERATIONS );

        //we've already satisfied everything
        if( bCorners && side1 && side2 )
            return;
    }
}


//check different parts of the given grid for the given color
void CubeWrapper::TestGridForColor( const GridSlot grid[][NUM_COLS], unsigned int color, bool &bCorners, bool &side1, bool &side2 )
{
    //only check for spots that haven't been found already
    if( !bCorners )
    {
        const Int2 cornerLocs[] = {
            Vec2( 0, 0 ),
            Vec2( 0, NUM_COLS - 1 ),
            Vec2( NUM_ROWS - 1, 0 ),
            Vec2( NUM_ROWS - 1, NUM_COLS - 1 )
        };

        for( int i = 0; i < 4; i++ )
        {
            const GridSlot &slot = grid[ cornerLocs[i].x ][ cornerLocs[i].y ];
            if( slot.matchesColor( color ) )
            {
                bCorners = true;
                break;
            }
        }
    }

    //side 1 looks like a spinning star spinning left
    /*
      XOXX
      XXXO
      OXXX
      XXOX

      only works with 4x4!
      */
    if( !side1 )
    {
        STATIC_ASSERT( ( NUM_ROWS == 4 ) && ( NUM_COLS == 4 ) );
        const Int2 locs[] = {
            Vec2( 0, 1 ),
            Vec2( 1, 3 ),
            Vec2( 2, 0 ),
            Vec2( 3, 2 )
        };

        for( int i = 0; i < 4; i++ )
        {
            const GridSlot &slot = grid[ locs[i].x ][ locs[i].y ];
            if( slot.matchesColor( color ) )
            {
                side1 = true;
                break;
            }
        }
    }

    //side 2 looks like a spinning star spinning right
    /*
      XXOX
      OXXX
      XXXO
      XOXX

      only works with 4x4!
      */
    if( !side2 )
    {
        const Int2 locs[] = {
            Vec2( 0, 2 ),
            Vec2( 1, 0 ),
            Vec2( 2, 3 ),
            Vec2( 3, 1 )
        };

        for( int i = 0; i < 4; i++ )
        {
            const GridSlot &slot = grid[ locs[i].x ][ locs[i].y ];
            if( slot.matchesColor( color ) )
            {
                side2 = true;
                break;
            }
        }
    }
}


//recursive function to tilt and test grid
void CubeWrapper::TiltAndTestGrid( GridSlot grid[][NUM_COLS], unsigned int color, bool &bCorners, bool &side1, bool &side2, int iterations )
{
    for( int i = 0; i < NUM_SIDES; i++ )
    {
        //copy the grid
        GridSlot childgrid[NUM_ROWS][NUM_COLS];

        //suddenly having problems with this memcpy
        //_SYS_memcpy8( (uint8_t *)childgrid, (uint8_t *)grid, sizeof( childgrid ) );
        for( int j = 0; j < NUM_ROWS; j++ )
        {
            for( int k = 0; k < NUM_COLS; k++ )
            {
                childgrid[j][k] = grid[j][k];
            }
        }

        //tilt it
        if( FakeTilt( i, childgrid ) )
        {
            TestGridForColor( childgrid, color, bCorners, side1, side2 );

            //we've already satisfied everything
            if( bCorners && side1 && side2 )
                return;

            //recurse
            if( iterations > 0 )
                TiltAndTestGrid( childgrid, color, bCorners, side1, side2, iterations - 1 );
        }
    }
}



bool CubeWrapper::HasFloatingDots() const
{
    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            const GridSlot &slot = m_grid[i][j];
            if( slot.isAlive() && !slot.IsFixed() )
                return true;
        }
    }

    return false;
}



//search for a multiplier dot and increase it
void CubeWrapper::UpMultiplier()
{
    for( int i = 1; i < NUM_ROWS - 1; i++ )
    {
        for( int j = 1; j < NUM_COLS - 1; j++ )
        {
            GridSlot &slot = m_grid[i][j];
            slot.UpMultiplier();
        }
    }
}


void CubeWrapper::ClearSprite( unsigned int id )
{
    m_vid.resizeSprite( id, 0, 0 );
}


void CubeWrapper::TurnOffSprites()
{
    ClearSprite( TimeKeeper::TIMER_SPRITE_NUM_ID );
    ClearSprite( GridSlot::MULT_SPRITE_ID );
    ClearSprite( GridSlot::MULT_SPRITE_NUM_ID );

    m_bubbles.Reset( m_vid );
}



void CubeWrapper::fillPuzzleCube()
{
    const PuzzleCubeData *pData = Game::Inst().GetPuzzleData( Game::Inst().getWrapperIndex( this ) );

    if( !pData )
        return;

    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_ROWS; j++ )
        {
            GridSlot &slot = m_grid[j][i];

            if( pData->m_aData[i][j] > 0 )
                slot.FillColor( pData->m_aData[i][j] - 1 );
            else
                slot.setEmpty();
        }
    }
}



//draw a message box with centered text
//bDrawBox - draw the box or not
//in_yOffset - optional y offset for text
void CubeWrapper::DrawMessageBoxWithText( const char *pTxt, bool bDrawBox, int in_yOffset )
{
    if( !m_dirty )
    {
        //TODO remove hack
        //for now just touch every frame
        //m_cube.vbuf.touch();
        return;
    }

    m_queuedFlush = true;
    m_dirty = false;

    if( bDrawBox )
        m_vid.BG0_drawAsset(Vec2(0,0), UI_BG, 0);

    //count how many lines of text we have
    int charCnt = 0;
    int index = 0;
    int lastspaceseen = -1;
    int numLines = 1;
    const int MAX_LINES = 8;
    const int MAX_LINE_LENGTH = 12;
    int lineBreakIndices[ MAX_LINES ];

    lineBreakIndices[0] = -1;

    while( pTxt[index] && pTxt[index] != '\n' )
    {
        if( pTxt[index] == ' ' )
            lastspaceseen = index;

        charCnt++;
        index++;

        //break at last space seen
        if( charCnt >= MAX_LINE_LENGTH - 2 )
        {
            ASSERT( lastspaceseen >= 0 );
            if( lastspaceseen < 0 )
            {
                return;
            }

            lineBreakIndices[ numLines ] = lastspaceseen;
            lastspaceseen = -1;
            charCnt = 0;
            numLines++;

            if( numLines > MAX_LINES )
            {
                ASSERT( 0 );
                return;
            }
        }
    }

    int yOffset = MAX_LINES - numLines + in_yOffset;

    for( int i = 0; i < numLines; i++ )
    {
        char aBuf[ MAX_LINE_LENGTH ];

        int endlineindex = i < numLines -1 ? lineBreakIndices[i + 1] : index;
        int length = endlineindex - lineBreakIndices[i];

        ASSERT( length >= 0 && length <= MAX_LINE_LENGTH );

        if( length < 0 || length > MAX_LINE_LENGTH )
            return;

        int xOffset = MAX_LINES - ( length / 2 );

        _SYS_strlcpy( aBuf, pTxt + lineBreakIndices[i] + 1, length );
        m_bg1helper.DrawText( Vec2( xOffset, yOffset ), WhiteFont, aBuf );

        yOffset += 2;
    }
}



void CubeWrapper::DrawGrid()
{
    ASSERT( m_numQueuedClears <= NUM_ROWS * NUM_COLS && m_numQueuedClears >= 0 );

    //flush our queued clears
    for( int i = 0; i < m_numQueuedClears; i++ )
    {
        m_vid.BG0_drawAsset(m_queuedClears[i], GemEmpty, 0);
    }

    ClearSprite( GridSlot::MULT_SPRITE_ID );
    ClearSprite( GridSlot::MULT_SPRITE_NUM_ID );

    //draw grid
    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            GridSlot &slot = m_grid[i][j];
            slot.Draw( m_vid, m_bg1helper, m_curFluidDir );
        }
    }
}



void CubeWrapper::StopGlimmer()
{
    static const float GLIMMER_BUFFER_TIME = 2.0f;
    m_glimmer.Stop();
    m_timeTillGlimmer += GLIMMER_BUFFER_TIME;
}


void CubeWrapper::SpawnRockExplosion( const Int2 &pos, unsigned int health )
{
    //find an unused explosion.
    for( int i = 0; i < RockExplosion::MAX_ROCK_EXPLOSIONS; i++ )
    {
        if( m_aExplosions[ i ].isUnused() )
        {
            m_aExplosions[ i ].Spawn( pos, health );
            return;
        }
    }

    //if we didn't find one, find oldest explosion
    int oldest = -1;
    unsigned int oldestFrame = 0;

    for( int i = 0; i < RockExplosion::MAX_ROCK_EXPLOSIONS; i++ )
    {
        if( m_aExplosions[ i ].getAnimFrame() > oldestFrame )
        {
            oldestFrame = m_aExplosions[ i ].getAnimFrame();
            oldest = i;
        }
    }

    m_aExplosions[ oldest ].Spawn( pos, health );
}


//each cube can have one floating score at a time
void CubeWrapper::SpawnScore( unsigned int score, const Int2 &slotpos )
{
    //find position where we should display score
    Int2 pos = { slotpos.y * 4 + 1, slotpos.x * 4 + 1 };

    m_floatscore.Spawn( score, pos );
}
