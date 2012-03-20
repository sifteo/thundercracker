#include "Game.h"

void Game::onNeighbor(
	void *context,
    Cube::ID c0, Cube::Side s0, 
    Cube::ID c1, Cube::Side s1) {
	sNeighborDirty = true;
}

void Game::MainLoop() {
  	//---------------------------------------------------------------------------
  	// INTRO

	for(Cube* p=gCubes; p!=gCubes+NUM_CUBES; ++p) {
		VidMode(p->vbuf).setWindow(0, 128);
	}

	#if FAST_FORWARD
	Cube* pPrimary = gCubes;
	#else
	PlayMusic(music_sting, false);
	Cube* pPrimary = IntroCutscene();
	#endif

	//---------------------------------------------------------------------------
  	// RESET EVERYTHING
	pInventory = 0;
	pMinimap = 0;
	mAnimFrames = 0;
	mNeedsSync = 0;
	mState.Init();
	mMap.Init();
	mPlayer.Init(pPrimary);
	for(ViewSlot* v = ViewBegin(); v!=ViewEnd(); ++v) { 
		if (v->GetCube() != pPrimary) { v->Init(); }
	}
	Zoom(mPlayer.View(), mPlayer.GetRoom()->Id());
	mPlayer.View()->ShowLocation(mPlayer.Location(), true);
	PlayMusic(music_castle);
	mSimTime = System::clock();
	_SYS_setVector(_SYS_NEIGHBOR_ADD, (void*) onNeighbor, NULL);
	_SYS_setVector(_SYS_NEIGHBOR_REMOVE, (void*) onNeighbor, NULL);
	CheckMapNeighbors();

	mIsDone = false;
	while(!mIsDone) {

    	//-------------------------------------------------------------------------
    	// WAIT FOR TOUCH
		mPlayer.SetStatus(PLAYER_STATUS_IDLE);
		mPlayer.CurrentView()->UpdatePlayer();
		mPath.Cancel();
		do {
			Paint();
			if (mPlayer.CurrentView()->Parent()->Touched()) {
				if (mPlayer.Equipment()) {
					OnUseEquipment();
				} else if (mPlayer.GetRoom()->HasItem()) {
					OnPickup(mPlayer.GetRoom());
				} else {
					OnActiveTrigger();
				}
			} else if (mPlayer.CurrentView()->GatewayTouched()) {
				OnEnterGateway(mPlayer.CurrentView()->GetRoom());
			}
      		#if PLAYTESTING_HACKS
          	else if (sShakeTime > 2.0f) {
	          	sShakeTime = -1.f;
	          	mState.AdvanceQuest();
	          	const QuestData* quest = mState.Quest();
	          	const MapData& map = gMapData[quest->mapId];
	          	const RoomData& room = map.rooms[quest->roomId];
	          	mPlayer.SetEquipment(0);
	          	TeleportTo(
	          		map, 
	          		Vec2(
	          			128 * (quest->roomId % map.width) + 16 * room.centerX,
	          			128 * (quest->roomId / map.width) + 16 * room.centerY
	          			)
	          		);
	          	mPlayer.SetStatus(PLAYER_STATUS_IDLE);
	          	mPlayer.CurrentView()->UpdatePlayer();

	        }
	      	#endif
    	} while (!gGame.GetMap()->FindBroadPath(&mPath));

	    //-------------------------------------------------------------------------
	    // PROCEED TO TARGET
	    mPlayer.SetStatus(PLAYER_STATUS_WALKING);
	    do {
	      	// animate walking to target
	      	mPlayer.SetDirection(mPath.steps[0]);
	      	mMap.GetBroadLocationNeighbor(*mPlayer.Current(), mPlayer.Direction(), mPlayer.Target());
	      	PlaySfx(sfx_running);
	      	mPlayer.TargetView()->ShowPlayer();
	      	// TODO: Walking South Through Door?
	      	if (mPlayer.Direction() == SIDE_TOP && mPlayer.CurrentRoom()->HasClosedDoor()) {

	        	//---------------------------------------------------------------------
	        	// WALKING NORTH THROUGH DOOR
	        	// walk up to the door
	      		int progress;
	      		STATIC_ASSERT(24 % WALK_SPEED == 0);
	      		for(progress=0; progress<24; progress+=WALK_SPEED) {
	      			mPlayer.Move(0, -WALK_SPEED);
	      			Paint();
	      		}
	      		if (mPlayer.HasBasicKey()) {
	      			// use the key and open the door
	      			mPlayer.UseBasicKey();
	      			mPlayer.CurrentRoom()->OpenDoor();
	      			mPlayer.CurrentView()->DrawBackground();
	      			mPlayer.CurrentView()->HideEquip();
	      			float timeout = System::clock();
	          		#if GFX_ARTIFACT_WORKAROUNDS
	      			Paint(true);
	      			mPlayer.CurrentView()->Parent()->GetCube()->vbuf.touch();
	          		#endif
	      			Paint(true);
	      			do { Paint(); } while(System::clock() - timeout <  0.5f);
	      			PlaySfx(sfx_doorOpen);
	          		// finish up
	      			for(; progress+WALK_SPEED<=128; progress+=WALK_SPEED) {
	      				mPlayer.Move(0,-WALK_SPEED);
	      				Paint();
	      			}
	          		// fill in the remainder
	      			mPlayer.SetPosition(
	      				mPlayer.GetRoom()->Center(mPlayer.Target()->subdivision)
	      			);
	      		} else {
	      			// buzz and return to the center of the current cube
	      			PlaySfx(sfx_doorBlock);
	      			mPath.Cancel();
	      			mPlayer.ClearTarget();
	      			mPlayer.SetDirection( (mPlayer.Direction()+2)%4 );
	      			for(progress=0; progress<24; progress+=WALK_SPEED) {
	      				Paint();
	      				mPlayer.Move(0, WALK_SPEED);
	      			}          
	      		}
	      	} else { 

	        	//---------------------------------------------------------------------
	        	// A* PATHFINDING
	      		if (mPlayer.TargetView()->GetRoom()->IsBridge()) {
	      			const Cube::Side hideParity = 
	      				mPlayer.TargetView()->GetRoom()->SubdivType() == SUBDIV_BRDG_HOR ? 1 : 0;
	      			mPlayer.TargetView()->HideOverlay(mPlayer.Direction()%2 == hideParity);
	      		}
	      		const bool foundPath = gGame.GetMap()->FindNarrowPath(*mPlayer.Current(), mPlayer.Direction(), &mMoves);
	      		ASSERT(foundPath);
	      		int progress = 0;
	      		// iterate through the tiles in the path
	      		Sokoblock* block = mPlayer.TargetView()->Block();
	      		bool pushing = false;
	      		
	      		// this loop could possibly suffer some optimizaton
	      		// but first I'm goint to wait until after alpha and not
	      		// more crap is going to get shoved in there
	      		for(const Cube::Side *pNextMove=mMoves.Begin(); pNextMove!=mMoves.End(); ++pNextMove) {
	      			// encounter lava?
	      			if (mMap.Data()->lavaTiles && !mPlayer.CanCrossLava()) {
	      				if (TryEncounterLava(*pNextMove)) {
	      					for(; pNextMove!=mMoves.Begin(); --pNextMove) {
	      						progress = MovePlayerOneTile(((*(pNextMove-1))+2)%4, progress);
	      					}
	      					mPath.Cancel();
	      					mPlayer.ClearTarget();
	      					break;
	      				}
	      			}
	      			// encounter a block?
	      			if(!pushing && block && mPlayer.TestCollision(block)) {
	      				pushing = TryEncounterBlock(block);
	      				if (!pushing) {
	      					// rewind back to the start
	      					for(; pNextMove!=mMoves.Begin(); --pNextMove) {
	      						progress = MovePlayerOneTile(((*(pNextMove-1))+2)%4, progress);
	      					}
	      					mPath.Cancel();
	      					mPlayer.ClearTarget();
	      					break;
	      				}
	      			}
      				progress = MovePlayerOneTile(*pNextMove, progress, pushing?block:0);
	      		}

	      		if (pushing) {
	      			// finish moving the block to the next cube
	      			const Room* targRoom = mMap.GetRoom(mPlayer.TargetRoom()->Location() + BroadDirection());
	      			const Vec2 targ = targRoom->Center(0);
	      			Vec2 curr= block->Position();
	      			while(curr != targ) {
	      				curr.x = AdvanceTowards(curr.x, targ.x, WALK_SPEED);
	      				curr.y = AdvanceTowards(curr.y, targ.y, WALK_SPEED);
	      				MoveBlock(block, curr - block->Position());
	      				Paint();
	      			}
	      			mPlayer.TargetView()->HideBlock();
			    	mPlayer.AdvanceToTarget();
	      			mPath.Cancel();
	      		}

	      	}
		    if (mPlayer.TargetView()) { // did we land on the target?
		    	mPlayer.AdvanceToTarget();
		      	if (OnPassiveTrigger() == TRIGGER_RESULT_PATH_INTERRUPTED) {
		      		mPath.Cancel();
		      	}
		    }
		} while(mPath.DequeueStep(*mPlayer.Current(), mPlayer.Target()));
  		OnActiveTrigger();
	}
	_SYS_setVector(_SYS_NEIGHBOR_ADD, NULL, NULL);
	_SYS_setVector(_SYS_NEIGHBOR_REMOVE, NULL, NULL);
	for(unsigned i=0; i<64; ++i) { 
		System::paint(); 
	}
	PlayMusic(music_winscreen, false);
	WinScreen();
}
