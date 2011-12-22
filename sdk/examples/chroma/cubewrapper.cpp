/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "cubewrapper.h"
#include "game.h"
#include "assets.gen.h"
//#include "audio.gen.h"
#include "utils.h"
#include "string.h"
#include <vector>
#include "sprite.h"

static _SYSCubeID s_id = 0;

// Order in which the number of colors in a grid increases as the grid is refilled.
static unsigned int GEM_VALUE_PROGRESSION[] = { 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

// Order in which the number of fixed gems in a grid increases as the grid is refilled.
static unsigned int GEM_FIX_PROGRESSION[] = { 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4 };

const float CubeWrapper::SHAKE_FILL_DELAY = 1.0f;
const float CubeWrapper::SPRING_K_CONSTANT = 0.7f;
const float CubeWrapper::SPRING_DAMPENING_CONSTANT = 0.07f;
const float CubeWrapper::MOVEMENT_THRESHOLD = 4.7f;
//const float CubeWrapper::IDLE_TIME_THRESHOLD = 3.0f;
//const float CubeWrapper::IDLE_FINISH_THRESHOLD = IDLE_TIME_THRESHOLD + ( GridSlot::NUM_IDLE_FRAMES * GridSlot::NUM_FRAMES_PER_IDLE_ANIM_FRAME * 1 / 60.0f );
const float CubeWrapper::MIN_GLIMMER_TIME = 20.0f;
const float CubeWrapper::MAX_GLIMMER_TIME = 30.0f;
const float CubeWrapper::TIME_PER_MESSAGE_FRAME = 0.25f / NUM_MESSAGE_FRAMES;
const float CubeWrapper::TILT_SOUND_EPSILON = 1.0f;



static const Sifteo::AssetImage *MESSAGE_IMGS[CubeWrapper::NUM_MESSAGE_FRAMES] = {
    &MessageBox0,
    &MessageBox1,
    &MessageBox2,
    &MessageBox3,
    &MessageBox4,
};


CubeWrapper::CubeWrapper() : m_cube(s_id++), m_vid(m_cube.vbuf), m_rom(m_cube.vbuf),
        m_bg1helper( m_cube ), m_state( STATE_PLAYING ), m_ShakesRemaining( STARTING_SHAKES ),
        m_fShakeTime( -1.0f ), m_curFluidDir( 0, 0 ), m_curFluidVel( 0, 0 ), m_stateTime( 0.0f ),
        m_lastTiltDir( 0 ), m_queuedFlush( false )
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

	//Refill();
}


void CubeWrapper::Init( AssetGroup &assets )
{
	m_cube.enable();
    m_cube.loadAssets( assets );

    m_rom.init();
    m_rom.BG0_text(Vec2(1,1), "Loading...");
    //Refill();
}


void CubeWrapper::Reset()
{
	m_ShakesRemaining = STARTING_SHAKES;
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
	Refill();
}

bool CubeWrapper::DrawProgress( AssetGroup &assets )
{
	m_rom.BG0_progressBar(Vec2(0,7), m_cube.assetProgress(GameAssets, m_vid.LCD_width), 2);
        
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
            m_intro.Draw( Game::Inst().getTimer(), m_bg1helper, m_cube, this );
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
                    m_vid.clear(GemEmpty.tiles[0]);
					//draw grid
					for( int i = 0; i < NUM_ROWS; i++ )
					{
						for( int j = 0; j < NUM_COLS; j++ )
						{
							GridSlot &slot = m_grid[i][j];
                            slot.Draw( m_vid, m_curFluidDir );
						}
					}

                    //draw glimmer before timer
                    m_glimmer.Draw( m_bg1helper, this );

                    if( Game::Inst().getMode() == Game::MODE_TIMED )
                    {
                        Game::Inst().getTimer().Draw( m_bg1helper );
                    }
                    if( m_banner.IsActive() )
                        m_banner.Draw( m_bg1helper );


                    m_queuedFlush = true;

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
                    m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);
                    m_bg1helper.DrawText( Vec2( 3, 3 ), Font, "SHAKE TO" );
                    m_bg1helper.DrawText( Vec2( 4, 5 ), Font, "REFILL" );

                    m_bg1helper.DrawTextf( Vec2( 4, 9 ), Font, "%d PTS", Game::Inst().getScore() );
                    m_queuedFlush = true;
					break;
				}
				case STATE_NOSHAKES:
				{
                    m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);
                    m_bg1helper.DrawText( Vec2( 4, 4 ), Font, "NO SHAKES" );
                    m_bg1helper.DrawText( Vec2( 4, 6 ), Font, "LEFT" );

                    m_queuedFlush = true;
					break;
				}
                case STATE_REFILL:
                {
                    m_intro.Draw( Game::Inst().getTimer(), m_bg1helper, m_cube, this );
                    m_queuedFlush = true;
                    break;
                }
			}			
			break;
		}
        case Game::STATE_DYING:
        {
            m_gameover.Draw( m_cube );
            m_queuedFlush = true;
            break;
        }
		case Game::STATE_POSTGAME:
		{
            m_vid.BG0_drawAsset(Vec2(0,0), MessageBox4, 0);

            if( m_cube.id() == 0 )
            {
                m_bg1helper.DrawText( Vec2( 3, 3 ), Font, "GAME OVER" );

                int score = Game::Inst().getScore();
                int len = score > 0 ? log10( score ) + 5 : 6;
                int xPos = ( Banner::BANNER_WIDTH - len ) / 2;

                m_bg1helper.DrawTextf( Vec2( xPos, 7 ), Font, "%d PTS", Game::Inst().getScore() );
            }
            else if( m_cube.id() == 1 )
            {
                m_bg1helper.DrawText( Vec2( 2, 2 ), Font, "HIGH SCORES" );

                for( unsigned int i = 0; i < Game::NUM_HIGH_SCORES; i++ )
                {
                    int score = Game::Inst().getHighScore(i);
                    int len = score > 0 ? log10( score ) + 1 : 2;
                    int xPos = 9 - len;

                    m_bg1helper.DrawTextf( Vec2( xPos, 5+2*i  ), Font, "%d", Game::Inst().getHighScore(i) );
                }
            }

            for( int i = 0; i < GameOver::NUM_ARROWS; i++ )
                resizeSprite(m_cube, i, 0, 0);

            m_queuedFlush = true;

			break;
		}
		default:
			break;
	}
	
}


void CubeWrapper::Update(float t, float dt)
{
    m_stateTime += dt;

    if( Game::Inst().getState() == Game::STATE_INTRO || m_state == STATE_REFILL )
    {
        if( !m_intro.Update( dt ) )
        {
            if( m_state == STATE_REFILL )
                m_state = STATE_PLAYING;
        }
        return;
    }
    else if( Game::Inst().getState() == Game::STATE_DYING )
    {
        m_gameover.Update( dt );
        return;
    }
    else if( Game::Inst().getState() == Game::STATE_PLAYING )
    {
        m_timeTillGlimmer -= dt;

        if( m_timeTillGlimmer < 0.0f )
        {
            m_timeTillGlimmer = Game::RandomRange( MIN_GLIMMER_TIME, MAX_GLIMMER_TIME );
            m_glimmer.Reset();
        }
        m_glimmer.Update( dt );
    }

    if( m_state == STATE_MESSAGING )
    {
        if( m_stateTime / TIME_PER_MESSAGE_FRAME >= NUM_MESSAGE_FRAMES )
            setState( STATE_EMPTY );
    }

    if( Game::Inst().getState() == Game::STATE_PLAYING )
    {
        //check for shaking
        if( m_state != STATE_NOSHAKES )
        {
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

            m_banner.Update(t, m_cube);
        }

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
				m_neighbors[i] = id;

                //PRINT( "neighbor on side %d is %d", i, id );
			}
		}
		else
			m_neighbors[i] = -1;
	}
}


void CubeWrapper::vidInit()
{
	m_vid.init();
}


void CubeWrapper::Tilt( int dir )
{
	bool bChanged = false;

	PRINT( "tilting" );

	if( m_fShakeTime > 0.0f )
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

	//change all pending movers to movers
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			slot.startPendingMove();
		}
	}

	if( bChanged )
		Game::Inst().setTestMatchFlag();

    m_lastTiltDir = dir;
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

    if( !dest.isOccupiable() )
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
		if( m_neighbors[i] >= 0 && m_neighbors[i] < m_cube.id() )
		{
			//as long we we test one block going clockwise, and the other going counter-clockwise, we'll match up
			int side = Game::Inst().cubes[m_neighbors[i]].GetSideNeighboredOn( 0, m_cube );

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

			CubeWrapper &otherCube = Game::Inst().cubes[m_neighbors[i]];
			otherCube.FillSlotArray( theirGems, side, false );

			//compare the two
			for( int j = 0; j < NUM_ROWS; j++ )
			{
                if( ourGems[j]->isMatchable() && theirGems[j]->isMatchable() && ourGems[j]->getColor() == theirGems[j]->getColor() )
				{
					ourGems[j]->mark();
					theirGems[j]->mark();
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
		if( m_neighbors[i] == cube.id() )
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


bool CubeWrapper::isFull()
{
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			if( slot.isEmpty() )
				return false;
		}
	}

	return true;
}

bool CubeWrapper::isEmpty()
{
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
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
            setState( STATE_MESSAGING );
        else
            setState( STATE_PLAYING );
	}
	else if( isEmpty() )
	{
        setState( STATE_REFILL );
        m_intro.Reset( true );
		Refill( true );

		if( Game::Inst().getMode() == Game::MODE_SHAKES && Game::Inst().getScore() > 0 )
		{
			m_banner.SetMessage( "FREE SHAKE!" );
		}
	}
	else
	{
		if( Game::Inst().getMode() != Game::MODE_SHAKES )
		{
            setState( STATE_REFILL );
            m_intro.Reset( true );
            Refill( true );
		}
		else if( m_ShakesRemaining > 0 )
		{
            setState( STATE_REFILL );
            m_intro.Reset( true );
            Refill( true );
            m_ShakesRemaining--;

			if( m_ShakesRemaining == 0 )
				m_banner.SetMessage( "NO SHAKES LEFT" );
			else if( m_ShakesRemaining == 1 )
				m_banner.SetMessage( "1 SHAKE LEFT" );
			else
			{
                char buf[16];
                snprintf(buf, sizeof buf - 1, "%d SHAKES LEFT", m_ShakesRemaining );
                m_banner.SetMessage( buf );
			}
		}
		else
		{
            setState( STATE_NOSHAKES );

			for( int i = 0; i < NUM_ROWS; i++ )
			{
				for( int j = 0; j < NUM_COLS; j++ )
				{
					GridSlot &slot = m_grid[i][j];
					slot.setEmpty();
				}
			}

			Game::Inst().checkGameOver();
		}
	}
}
 

//massively ugly, but wanted to stick to the python functionality 
void CubeWrapper::Refill( bool bAddLevel )
{
	unsigned int level = Game::Inst().getLevel();
	unsigned int numValues = sizeof( GEM_VALUE_PROGRESSION ) / sizeof( GEM_VALUE_PROGRESSION[0] );
	unsigned int values = level < numValues ? GEM_VALUE_PROGRESSION[level] : GEM_VALUE_PROGRESSION[numValues - 1];
	unsigned int numFixIndices = sizeof( GEM_FIX_PROGRESSION ) / sizeof( GEM_FIX_PROGRESSION[0] );
    unsigned int numFix = level < numFixIndices ? GEM_FIX_PROGRESSION[level] : GEM_FIX_PROGRESSION[numFixIndices - 1];
	
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
	unsigned int aNumNeeded[GridSlot::NUM_COLORS];
	unsigned int iNumFixed = 0;

	memset( aNumNeeded, 0, GridSlot::NUM_COLORS * sizeof( int ) );

	int numDots = NUM_ROWS * NUM_COLS;

	for( unsigned int i = 0; i < values; i++ )
	{
		aNumNeeded[i] = numDots / values;
		if( i < numDots % values )
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
		unsigned int randIndex = Game::Inst().Rand( numEmpties );
		unsigned int temp = aLocIndices[randIndex];
		aLocIndices[randIndex] = aLocIndices[i];
		aLocIndices[i] = temp;
	}

	unsigned int iCurColor = 0;

	for( unsigned int i = 0; i < numEmpties; i++ )
	{
		int curX = aEmptyLocs[aLocIndices[i]].x;
		int curY = aEmptyLocs[aLocIndices[i]].y;
		GridSlot &slot = m_grid[curX][curY];

		if( slot.isAlive() )
			continue;

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
				int side = Game::Inst().Rand( NUM_SIDES );
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
	while( iNumFixed < numFix )
	{
		int toFix = Game::Inst().Rand( numEmpties );
		GridSlot &fix = m_grid[aEmptyLocs[toFix].x][aEmptyLocs[toFix].y];
		bool bFound = false;

        //TODO, this is only because we don't have all the colors yet!
        if( fix.getColor() >= GridSlot::NUM_FIXED_COLORS )
        {
            fix.FillColor( 0 );
        }

		if( !fix.IsFixed() )
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
			if( slot.isAlive() && slot.getColor() == color )
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
				//we have floating dots
				else
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
				//we have floating dots
				else
					return false;
			}
		}
	}

	return bHasFixed;
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
				else
					return false;
			}
		}
	}

	return count==1;
}


void CubeWrapper::checkEmpty()
{
	if( m_state != STATE_NOSHAKES && isEmpty() )
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
        Game::Inst().playSound(message_pop_03_fx);
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
