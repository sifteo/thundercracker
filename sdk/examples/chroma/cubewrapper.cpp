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
#include <vector>

static _SYSCubeID s_id = 0;

// Order in which the number of colors in a grid increases as the grid is refilled.
static unsigned int GEM_VALUE_PROGRESSION[] = { 3, 4, 4, 5, 5, 6, 6, 7, 7, 8 };

// Order in which the number of fixed gems in a grid increases as the grid is refilled.
static unsigned int GEM_FIX_PROGRESSION[] = { 0, 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 4 };

CubeWrapper::CubeWrapper() : m_cube(s_id++), m_vid(m_cube.vbuf), m_rom(m_cube.vbuf), m_flipsRemaining( STARTING_FLIPS ), m_fShakeTime( -1.0f )
{
	for( int i = 0; i < NUM_SIDES; i++ )
	{
		m_neighbors[i] = -1;
	}


	/*// Now enable BG0, Sprites, and BG1
    _SYS_vbuf_pokeb(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
	_SYS_vbuf_fill(&m_cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);

	// Allocate tiles for the banner
    _SYS_vbuf_fill(&m_cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_bitmap) / 2, 0xFFFF, BANNER_ROWS );*/

	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			slot.Init( this, i, j );
		}
	}

	m_state = STATE_PLAYING;
	//Refill();
}


void CubeWrapper::Init( AssetGroup &assets )
{
	m_cube.enable();
    m_cube.loadAssets( assets );

    m_rom.init();
    m_rom.BG0_text(Vec2(1,1), "Loading...");
	Refill();
}

bool CubeWrapper::DrawProgress( AssetGroup &assets )
{
	m_rom.BG0_progressBar(Vec2(0,7), m_cube.assetProgress(GameAssets, m_vid.LCD_width), 2);
        
	return m_cube.assetDone(assets);
}

void CubeWrapper::Draw()
{
	//draw grid
	for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			slot.Draw( m_vid, Vec2(j * 4, i * 4) );
		}
	}

	m_banner.Draw( m_cube );
}


void CubeWrapper::DrawSplash()
{
	m_vid.BG0_drawAsset(Vec2(0,0), Cover, 0);
}


void CubeWrapper::Update(float t)
{
	//check for shaking
	if( m_state != STATE_NOFLIPS )
	{
        if( m_fShakeTime > 0.0f && t - m_fShakeTime > SHAKE_FILL_DELAY )
		{
            m_fShakeTime = -1.0f;
            checkRefill();
		}
	}

	for( Cube::Side i = 0; i < NUM_SIDES; i++ )
	{
        bool newValue = m_cube.hasPhysicalNeighborAt(i);
		Cube::ID id = m_cube.physicalNeighborAt(i);

		if( newValue )
		{
			PRINT( "we have a neighbor.  it is %d\n", id );
		}

		//newly neighbored
		if( newValue )
		{
			if( id != m_neighbors[i] )
			{
				Game::Inst().setTestMatchFlag();
				m_neighbors[i] = id;

				PRINT( "neighbor on side %d is %d", i, id );
			}
		}
		else
			m_neighbors[i] = -1;
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

	if( bChanged )
		Game::Inst().setTestMatchFlag();
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

	if( slot.isAlive() )
	{
		dest = slot;
		slot.setEmpty();
		return true;
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
			//TEMP try marking everything
			/*for( int k = 0; k < NUM_ROWS; k++ )
			{
				for( int l = 0; l < NUM_ROWS; l++ )
				{
					if( m_grid[k][l].isAlive() )
						m_grid[k][l].mark();
				}
			}

			return;*/

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
				if( ourGems[j]->isAlive() && theirGems[j]->isAlive() && ourGems[j]->getColor() == theirGems[j]->getColor() )
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
            m_state = STATE_PLAYING;
    else if( Game::Inst().getMode() == Game::MODE_PUZZLE )
	{
        if( isEmpty() )
            m_state = STATE_EMPTY;
        else
            m_state = STATE_PLAYING;
	}
	else if( isEmpty() )
	{
		m_state = STATE_PLAYING;
		Refill( true );

		if( Game::Inst().getMode() == Game::MODE_FLIPS && Game::Inst().getScore() > 0 )
		{
			m_banner.SetMessage( "FREE FLIP!" );
		}
	}
	else
	{
		if( Game::Inst().getMode() != Game::MODE_FLIPS )
		{
			m_state = STATE_PLAYING;
            Refill( true );
		}
		else if( m_flipsRemaining > 0 )
		{
			m_state = STATE_PLAYING;
            Refill( true );
            m_flipsRemaining--;

			if( m_flipsRemaining == 0 )
				m_banner.SetMessage( "NO FLIPS LEFT" );
			else if( m_flipsRemaining == 1 )
				m_banner.SetMessage( "1 FLIP LEFT" );
			else
			{
				char aBuf[16];
				sprintf( aBuf, "%d FLIPS LEFT", m_flipsRemaining );
				m_banner.SetMessage( aBuf );
			}
		}
		else
		{
			m_state = STATE_NOFLIPS;
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

	//TODO SOUND
	//self.game.sound_manager.add("reload")

	/*for( int i = 0; i < NUM_ROWS; i++ )
	{
		for( int j = 0; j < NUM_COLS; j++ )
		{
			GridSlot &slot = m_grid[i][j];
			slot.Init( this, i, j );
		}
	}*/

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

	int iCurColor = 0;

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