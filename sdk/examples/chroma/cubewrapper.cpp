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
const float CubeWrapper::TIME_PER_MESSAGE_FRAME = 0.25f / NUM_MESSAGE_FRAMES;
const float CubeWrapper::TILT_SOUND_EPSILON = 5.0f;
//const float CubeWrapper::SHOW_BONUS_TIME = 3.1f;


static const Sifteo::AssetImage *MESSAGE_IMGS[CubeWrapper::NUM_MESSAGE_FRAMES] = {
    &MessageBox0,
    &MessageBox1,
    &MessageBox2,
    &MessageBox3,
};


CubeWrapper::CubeWrapper() : m_cube(s_id++), m_vid(m_cube.vbuf), m_rom(m_cube.vbuf),
        m_bg1helper( m_cube ), m_state( STATE_PLAYING ),
        m_fShakeTime( -1.0f ), m_curFluidDir( 0, 0 ), m_curFluidVel( 0, 0 ), m_stateTime( 0.0f ),
        m_lastTiltDir( 0 ), m_numQueuedClears( 0 ), m_queuedFlush( false ), m_dirty( true )
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
	m_fShakeTime = -1.0f;
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
    m_gameover.Reset();
    m_glimmer.Reset();
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
			m_vid.BG0_drawAsset(Vec2(0,0), Cover, 0);
			break;
		}
        case Game::STATE_INTRO:
        {
            m_intro.Draw( Game::Inst().getTimer(), m_bg1helper, m_vid, this );
            m_queuedFlush = true;
            break;
        }
		case Game::STATE_PLAYING:
		{
			switch( m_state )
			{
				case STATE_PLAYING:
				{
					//clear out grid first (somewhat wasteful, optimize if necessary)
                    //m_vid.clear(GemEmpty.tiles[0]);

                    ASSERT( m_numQueuedClears <= NUM_ROWS * NUM_COLS && m_numQueuedClears >= 0 );

                    //flush our queued clears
                    for( int i = 0; i < m_numQueuedClears; i++ )
                    {
                        m_vid.BG0_drawAsset(m_queuedClears[i], GemEmpty, 0);
                    }

					//draw grid
					for( int i = 0; i < NUM_ROWS; i++ )
					{
						for( int j = 0; j < NUM_COLS; j++ )
						{
							GridSlot &slot = m_grid[i][j];
                            slot.Draw( m_vid, m_bg1helper, m_curFluidDir );
						}
					}

                    //draw glimmer before timer
                    m_glimmer.Draw( m_bg1helper, this );

                    if( Game::Inst().getMode() == Game::MODE_TIMED )
                    {
                        Game::Inst().getTimer().Draw( m_bg1helper, m_vid );
                    }
                    if( m_banner.IsActive() )
                        m_banner.Draw( m_bg1helper );


                    m_queuedFlush = true;

                    //super debug code!
                    //Banner::DrawScore( m_bg1helper, Vec2( 0, 0 ), Banner::LEFT, m_cube.id() );

                    //for debugging combo count
                    //if( Game::Inst().getMode() == Game::MODE_TIMED )
                      //  Banner::DrawScore( m_bg1helper, Vec2( 0, 0 ), Banner::LEFT, Game::Inst().GetComboCount() );

					break;
				}
                case STATE_MESSAGING:
                {
                    int frame = m_stateTime / TIME_PER_MESSAGE_FRAME;

                    if( frame >= NUM_MESSAGE_FRAMES )
                        frame = NUM_MESSAGE_FRAMES - 1;
                    const Sifteo::AssetImage &img = *MESSAGE_IMGS[frame];
                    m_vid.BG0_drawAsset(Vec2(0,16 - img.height), img, 0);
                    break;
                }
				case STATE_EMPTY:
				{
                    m_vid.BG0_drawAsset(Vec2(0,0), MsgShakeToRefill, 0);
                    /*int level = Game::Inst().getDisplayedLevel();

                    Banner::DrawScore( m_bg1helper, Vec2( Banner::CENTER_PT, 10 ),
                                       Banner::CENTER, level );*/

                    /*m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);
                    m_bg1helper.DrawText( Vec2( 3, 3 ), Font, "SHAKE TO" );
                    m_bg1helper.DrawText( Vec2( 4, 5 ), Font, "REFILL" );

                    m_bg1helper.DrawTextf( Vec2( 4, 9 ), Font, "%d PTS", Game::Inst().getScore() );*/

                    //m_queuedFlush = true;
					break;
				}
                /*case STATE_CUBEBONUS:
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);

                    String<3> bufCubes;

                    unsigned int numCubes = Game::Inst().CountEmptyCubes();
                    bufCubes << numCubes;

                    m_bg1helper.DrawText( Vec2( 5, 3 ), Font, bufCubes );
                    m_bg1helper.DrawText( Vec2( 7, 3 ), Font, "Cube" );
                    m_bg1helper.DrawText( Vec2( 4, 5 ), Font, "Bonus:" );
                    Banner::DrawScore( m_bg1helper, Vec2( 7, 7 ), Banner::CENTER, numCubes * PTS_PER_EMPTIED_CUBE );

                    m_queuedFlush = true;
                    break;
                }*/

                case STATE_REFILL:
                {
                    m_intro.Draw( Game::Inst().getTimer(), m_bg1helper, m_vid, this );
                    m_queuedFlush = true;
                    break;
                }
			}			
			break;
		}
        case Game::STATE_DYING:
        {
            if( m_banner.IsActive() )
                m_banner.Draw( m_bg1helper );
            else
                m_gameover.Draw( m_vid );
            m_queuedFlush = true;
            break;
        }
		case Game::STATE_POSTGAME:
		{
            if( !m_dirty )
            {
                //force touch of a cube, maybe it'll fix things
                m_cube.vbuf.touch();
                break;
            }

            //m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);

            if( m_cube.id() == 0 + CUBE_ID_BASE)
            {
                m_vid.BG0_drawAsset(Vec2(0,0), MsgGameOver, 0);

                if( Game::Inst().getMode() == Game::MODE_TIMED )
                {
                    int score = Game::Inst().getScore();

                    //m_bg1helper.DrawText( Vec2( 3, 3 ), Font, "GAME OVER" );
                    //m_bg1helper.DrawTextf( Vec2( xPos, 7 ), Font, "%d PTS", Game::Inst().getScore() );
                    Banner::DrawScore( m_bg1helper, Vec2( Banner::CENTER_PT, 11 ),
                                       Banner::CENTER, score );
                }
                else
                {
                    m_vid.BG0_drawPartialAsset( Vec2( 0, 7), Vec2( 0, 7), Vec2( 16, 9), MessageBox4 );
                }
            }
            else if( m_cube.id() == 1 + CUBE_ID_BASE )
            {
                if( Game::Inst().getMode() == Game::MODE_TIMED )
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), MsgHighScores, 0);
                    //m_bg1helper.DrawText( Vec2( 2, 2 ), Font, "HIGH SCORES" );

                    for( unsigned int i = 0; i < Game::NUM_HIGH_SCORES; i++ )
                    {
                        int score = Game::Inst().getHighScore(i);

                        //m_bg1helper.DrawTextf( Vec2( xPos, 5+2*i  ), Font, "%d", Game::Inst().getHighScore(i) );
                        Banner::DrawScore( m_bg1helper, Vec2( 8, 5+2*i ), Banner::RIGHT, score );

                    }
                }
                else
                {
                    m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);
                    m_bg1helper.DrawText( Vec2( 4, 3 ), Font, "Reached" );
                    m_bg1helper.DrawText( Vec2( 5, 5 ), Font, "Level" );

                    Banner::DrawScore( m_bg1helper, Vec2( 7, 9 ), Banner::CENTER, Game::Inst().getDisplayedLevel() );
                }
            }
            else if( m_cube.id() == 2 + CUBE_ID_BASE )
            {
                m_vid.BG0_drawAsset(Vec2(0,0), MsgShakeNewGame, 0);
                //m_bg1helper.DrawTextf( Vec2( 4, 3 ), Font, "Shake or\nNeighbor\nfor new\n game" );
            }

            for( int i = 0; i < GameOver::NUM_ARROWS; i++ )
                m_vid.resizeSprite(i, 0, 0);

            m_queuedFlush = true;
            m_dirty = false;

			break;
		}
        case Game::STATE_GOODJOB:
        {
            DrawMessageBoxWithText( "Good Job" );
            break;
        }
		default:
			break;
	}

    m_numQueuedClears = 0;
	
}


void CubeWrapper::Update(float t, float dt)
{
    m_stateTime += dt;

    if( Game::Inst().getState() == Game::STATE_INTRO || m_state == STATE_REFILL )
    {
        if( !m_intro.Update( dt, m_banner ) )
        {
            if( m_state == STATE_REFILL )
                m_state = STATE_PLAYING;
        }
        return;
    }
    else if( Game::Inst().getState() == Game::STATE_DYING )
    {
        if( m_banner.IsActive() )
            m_banner.Update(t);
        else
            m_gameover.Update( dt );
        return;
    }
    else if( Game::Inst().getState() == Game::STATE_PLAYING )
    {
        m_timeTillGlimmer -= dt;

        if( m_timeTillGlimmer < 0.0f )
        {
            m_timeTillGlimmer = Game::random.uniform( MIN_GLIMMER_TIME, MAX_GLIMMER_TIME );
            m_glimmer.Reset();
        }
        m_glimmer.Update( dt );
    }

    if( m_state == STATE_MESSAGING )
    {
        if( m_stateTime / TIME_PER_MESSAGE_FRAME >= NUM_MESSAGE_FRAMES )
        {
            int count = Game::Inst().CountEmptyCubes();
            //bonuses for multiple cubes being empty
            /*if( count > 1 )
            {
                setState( STATE_CUBEBONUS );
                Game::Inst().addScore( count * PTS_PER_EMPTIED_CUBE );
            }
            else*/
                setState( STATE_EMPTY );
        }
    }
    /*else if( m_state == STATE_CUBEBONUS )
    {
        if( m_stateTime > SHOW_BONUS_TIME )
            setState( STATE_EMPTY );
    }*/

    if( Game::Inst().getState() == Game::STATE_PLAYING )
    {
        //check for shaking
        if( m_fShakeTime > 0.0f && t - m_fShakeTime > SHAKE_FILL_DELAY )
        {
            m_fShakeTime = -1.0f;
            checkRefill();
        }

        //update all dots
        for( int i = 0; i < NUM_ROWS; i++ )
        {
            for( int j = 0; j < NUM_COLS; j++ )
            {
                GridSlot &slot = m_grid[i][j];
                slot.Update( t );
            }
        }

        m_banner.Update(t);

        //tilt state
        _SYSAccelState state;
        _SYS_getAccel(m_cube.id(), &state);

        //try spring to target
        Float2 delta = Float2( state.x, state.y ) - m_curFluidDir;

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
        m_curFluidDir += m_curFluidVel * dt;

        if( m_curFluidVel.x > TILT_SOUND_EPSILON || m_curFluidVel.y > TILT_SOUND_EPSILON )
        {
            if( oldvel.x * m_curFluidVel.x < 0.0f || oldvel.y * m_curFluidVel.y < 0.0f )
                Game::Inst().playSlosh();
        }

        for( Cube::Side i = 0; i < NUM_SIDES; i++ )
        {
            bool newValue = m_cube.hasPhysicalNeighborAt(i);
            Cube::ID id = m_cube.physicalNeighborAt(i);

            /*if( newValue )
            {
                PRINT( "we have a neighbor.  it is %d\n", id );
            }*/

            //newly neighbored
            if( newValue )
            {
                if( id != m_neighbors[i] )
                {
                    Game::Inst().setTestMatchFlag();
                    m_neighbors[i] = id - CUBE_ID_BASE;

                    //PRINT( "neighbor on side %d is %d", i, id );
                }
            }
            else
                m_neighbors[i] = -1;
        }
    }
    else if( Game::Inst().getState() == Game::STATE_POSTGAME )
    {
        if( m_fShakeTime > 0.0f && t - m_fShakeTime > SHAKE_FILL_DELAY )
        {
            Game::Inst().setTestMatchFlag();
            m_fShakeTime = -1.0f;
        }
    }
}


void CubeWrapper::vidInit()
{
	m_vid.init();
}


void CubeWrapper::Tilt( int dir )
{
	bool bChanged = false;

    if( Game::Inst().getState() != Game::STATE_PLAYING || m_fShakeTime > 0.0f )
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

    if( bChanged )
    {
        //change all pending movers to movers
        for( int i = 0; i < NUM_ROWS; i++ )
        {
            for( int j = 0; j < NUM_COLS; j++ )
            {
                GridSlot &slot = grid[i][j];
                slot.finishFakeMove();
            }
        }
    }

    return bChanged;
}


void CubeWrapper::Shake( bool bShaking )
{
	if( bShaking )
		m_fShakeTime = System::clock();
	else
		m_fShakeTime = -1.0f;
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
                    else if( ourGems[j]->getColor() == theirGems[j]->getColor() )
                    {
                        ourGems[j]->mark();
                        theirGems[j]->mark();
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
        if( isEmpty() )
        {
            if( m_state != STATE_MESSAGING )
                setState( STATE_MESSAGING );
        }
        else
            setState( STATE_PLAYING );
	}
	else if( isEmpty() )
	{
        setState( STATE_REFILL );
        m_intro.Reset( true );
        Refill( Game::Inst().getMode() == Game::MODE_SHAKES );

        /*if( Game::Inst().getMode() == Game::MODE_SHAKES && Game::Inst().getScore() > 0 )
		{
			m_banner.SetMessage( "FREE SHAKE!" );
        }*/
	}
	else
	{
		if( Game::Inst().getMode() != Game::MODE_SHAKES )
		{
            setState( STATE_REFILL );
            m_intro.Reset( true );
            Refill( false );
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
void CubeWrapper::Refill( bool bAddLevel )
{
    if( Game::Inst().getMode() == Game::MODE_PUZZLE )
    {
        fillPuzzleCube();
        return;
    }

    const Level &level = Game::Inst().getLevel();
	
	if( bAddLevel )
		Game::Inst().addLevel();

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

	unsigned int numEmpties = 0;
	Vec2 aEmptyLocs[NUM_ROWS * NUM_COLS];

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
				aEmptyLocs[numEmpties++] = Vec2( i, j );
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
    Vec2 aEmptyLocs[NUM_ROWS * NUM_COLS];

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
bool CubeWrapper::getFixedDot( Vec2 &pos ) const
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
    if( isEmpty() && m_state != STATE_MESSAGING )
        setState( STATE_MESSAGING );
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

    if( state == STATE_MESSAGING )
    {
        Game::Inst().playSound(message_pop_03_fx);
    }
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


void CubeWrapper::QueueClear( Vec2 &pos )
{
    m_queuedClears[m_numQueuedClears] = pos;
    m_numQueuedClears++;
    ASSERT( m_numQueuedClears <= NUM_ROWS * NUM_COLS );
}


//look for an empty spot to put a hyper dot
void CubeWrapper::SpawnSpecial( unsigned int color )
{
    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_COLS; j++ )
        {
            GridSlot &slot = m_grid[i][j];

            if( slot.isEmpty() )
            {
                slot.FillColor( color );
                return;
            }
        }
    }
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

        _SYS_memcpy8( (uint8_t *)grid, (uint8_t *)m_grid, sizeof( grid ) );

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
        const Vec2 cornerLocs[] = {
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
        const Vec2 locs[] = {
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
        const Vec2 locs[] = {
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

        _SYS_memcpy8( (uint8_t *)childgrid, (uint8_t *)grid, sizeof( childgrid ) );

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



void CubeWrapper::fillPuzzleCube()
{
    const PuzzleCubeData *pData = Game::Inst().GetPuzzleData( m_cube.id() - CUBE_ID_BASE );

    if( !pData )
        return;

    for( int i = 0; i < NUM_ROWS; i++ )
    {
        for( int j = 0; j < NUM_ROWS; j++ )
        {
            GridSlot &slot = m_grid[j][i];

            ASSERT( !slot.isAlive() );

            if( pData->m_aData[i][j] > 0 )
                slot.FillColor( pData->m_aData[i][j] - 1 );
        }
    }
}



//draw a message box with centered text
void CubeWrapper::DrawMessageBoxWithText( const char *pTxt )
{
    m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);

    //count how many lines of text we have
    int charCnt = 0;
    int index = 0;
    int lastspaceseen = -1;
    int numLines = 1;
    const int MAX_LINES = 8;
    const int MAX_LINE_LENGTH = 16;
    int lineBreakIndices[ MAX_LINES ];

    lineBreakIndices[0] = -1;

    while( pTxt[index] && pTxt[index] != '\n' )
    {
        if( pTxt[index] == ' ' )
            lastspaceseen = index;

        charCnt++;
        index++;

        //break at last space seen
        if( charCnt >= MAX_LINE_LENGTH )
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

    int yOffset = MAX_LINES - numLines;

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
        m_bg1helper.DrawText( Vec2( xOffset, yOffset ), Font, aBuf );

        yOffset += 2;
    }

    m_queuedFlush = true;
}
