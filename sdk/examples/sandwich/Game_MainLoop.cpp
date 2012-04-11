#include "Game.h"

void Game::OnNeighbor(unsigned c0, unsigned s0, unsigned c1, unsigned s1) {
	gGame.mNeighborDirty = true;
}

void Game::MainLoop() {
	ASSERT(this == &gGame);
	
	mActiveViewMask = CUBE_ALLOC_MASK;
	mLockedViewMask = 0x00000000;

  	//---------------------------------------------------------------------------
  	// INTRO
	for(CubeID c=0; c<NUM_CUBES; ++c) {
		ViewAt(c)->Video().attach(c);
		ViewAt(c)->Video().initMode(BG0_SPR_BG1);
	}

	#if FAST_FORWARD
		VideoBuffer* pPrimary = &ViewAt(0)->Video();
	#else
		PlayMusic(music_sting, false);
		VideoBuffer* pPrimary = IntroCutscene();
	#endif

	//---------------------------------------------------------------------------
  	// RESET EVERYTHING
	mNeighborDirty = false;
	mPrevTime = SystemTime::now();
	pInventory = 0;
	pMinimap = 0;
	mAnimFrames = 0;
	mNeedsSync = 0;
	mState.Init();
	mMap.Init();
	mPlayer.Init(pPrimary);
	Viewport::Iterator p = ListViews();
	while(p.MoveNext()) {
		if (&(p->Video()) != pPrimary) { p->Init(); }
	}
	Zoom(mPlayer.View(), mPlayer.GetRoom()->Id());
	mPlayer.View()->ShowLocation(mPlayer.Location(), true);
	PlayMusic(music_castle);
	Events::neighborAdd.set(&Game::OnNeighbor, this);
    Events::neighborRemove.set(&Game::OnNeighbor, this);
	CheckMapNeighbors();

	mIsDone = false;
	while(!mIsDone) {

    	//-------------------------------------------------------------------------
    	// WAIT FOR TOUCH
		mPlayer.SetStatus(PLAYER_STATUS_IDLE);
		mPlayer.CurrentView()->UpdatePlayer();
		mPath.Cancel();
		unsigned targetViewId = 0xff;
		while(targetViewId == 0xff) {
			Paint();
			OnTick();
			if (mPlayer.CurrentView()->Parent()->Touched()) {
				if (mPlayer.Equipment()) {
					OnUseEquipment();
				} else if (mPlayer.GetRoom()->HasItem()) {
					OnPickup(mPlayer.GetRoom());
				} else {
					OnActiveTrigger();
				}
			} else if (mPlayer.CurrentView()->GatewayTouched()) {
				OnEnterGateway(mPlayer.CurrentView()->GetRoom()->Gateway());
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
	          		vec(
	          			128 * (quest->roomId % map.width) + 16 * room.centerX,
	          			128 * (quest->roomId / map.width) + 16 * room.centerY
	          		)
	          	);
	          	mPlayer.SetStatus(PLAYER_STATUS_IDLE);
	          	mPlayer.CurrentView()->UpdatePlayer();

	        }
	      	#endif
	      	if (!gGame.GetMap()->FindBroadPath(&mPath, &targetViewId)) {
	      		Viewport::Iterator p = ListViews();
				while(p.MoveNext()) {
	      			if ( p->Touched() && p->ShowingRoom() && p->GetRoomView() != mPlayer.CurrentView()) {
	      				p->GetRoomView()->StartShake();
	      				//p->GetRoomView()->Lock();
	      			}
	      		}
	      	}
    	}
    	if (mViews[targetViewId].ShowingRoom()) {
    		mViews[targetViewId].GetRoomView()->StartNod();
    	}

	    //-------------------------------------------------------------------------
	    // PROCEED TO TARGET
	    mPlayer.SetStatus(PLAYER_STATUS_WALKING);
	    do {
	      	// animate walking to target
	      	mPlayer.SetDirection((Side)mPath.steps[0]);
	      	mMap.GetBroadLocationNeighbor(*mPlayer.Current(), mPlayer.Direction(), mPlayer.Target());
	      	PlaySfx(sfx_running);
	      	mPlayer.TargetView()->ShowPlayer();
	      	// TODO: Walking South Through Door?
	      	if (mPlayer.Direction() == TOP && mPlayer.CurrentRoom()->HasClosedDoor()) {

	        	//---------------------------------------------------------------------
	        	// WALKING NORTH THROUGH DOOR
	        	// walk up to the door
	        	const DoorData& door = *mPlayer.CurrentRoom()->Door();
	      		int progress;
	      		STATIC_ASSERT(24 % WALK_SPEED == 0);
	      		for(progress=0; progress<24; progress+=WALK_SPEED) {
	      			mPlayer.Move(0, -WALK_SPEED);
	      			Paint();
	      		}
	      		if (door.keyItemId != 0xff && mPlayer.Equipment() && mPlayer.Equipment()->itemId == door.keyItemId) {
	      			// use the key and open the door
	      			mPlayer.ConsumeEquipment();
	      			mPlayer.CurrentRoom()->OpenDoor();
	      			mPlayer.CurrentView()->RefreshDoor();
	      			mPlayer.CurrentView()->HideEquip();
	      			Paint(true);
	      			Wait(0.5f);
	      			PlaySfx(sfx_doorOpen);
	          		// finish up
	      			for(; progress+WALK_SPEED<=128; progress+=WALK_SPEED) {
	      				Paint();
	      				mPlayer.Move(0,-WALK_SPEED);
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
	      			mPlayer.SetDirection( (Side)((mPlayer.Direction()+2)%4) );
	      			for(progress=0; progress<24; progress+=WALK_SPEED) {
	      				Paint();
	      				mPlayer.Move(0, WALK_SPEED);
	      			}          
	      		}
	      	} else { 

	        	//---------------------------------------------------------------------
	        	// A* PATHFINDING
	      		if (mPlayer.TargetView()->GetRoom()->IsBridge()) {
	      			const unsigned hideParity =
	      				mPlayer.TargetView()->GetRoom()->SubdivType() == SUBDIV_BRDG_HOR ? 1 : 0;
	      			mPlayer.TargetView()->HideOverlay(mPlayer.Direction()%2 == hideParity);
	      		}
	      		gGame.GetMap()->FindNarrowPath(*mPlayer.Current(), mPlayer.Direction(), &mMoves);
	      		int progress = 0;
	      		Sokoblock* block = mPlayer.TargetView()->Block();
	      		bool pushing = false;
	      		
	      		// iterate through the tiles in the path
	      		// this loop could possibly suffer some optimizaton
	      		// but first I'm goint to wait until after alpha and not
	      		// more crap is going to get shoved in there
	      		for(const int8_t *pNextMove=mMoves.Begin(); pNextMove!=mMoves.End(); ++pNextMove) {
	      			// encounter lava?
	      			if (mMap.Data()->lavaTiles && !mPlayer.CanCrossLava()) {
	      				if (TryEncounterLava((Side)*pNextMove)) {
	      					for(; pNextMove!=mMoves.Begin(); --pNextMove) {
	      						progress = MovePlayerOneTile((Side)(((*(pNextMove-1))+2)%4), progress);
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
	      						progress = MovePlayerOneTile((Side)(((*(pNextMove-1))+2)%4), progress);
	      					}
	      					mPath.Cancel();
	      					mPlayer.ClearTarget();
	      					break;
	      				}
	      			}
      				progress = MovePlayerOneTile((Side)*pNextMove, progress, pushing?block:0);
	      		}

	      		if (pushing) {
	      			// finish moving the block to the next cube
	      			const Room* targRoom = mMap.GetRoom(mPlayer.TargetRoom()->Location() + BroadDirection());
	      			const Int2 targ = targRoom->Center(0);
	      			Int2 curr= block->Position();
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
	Events::neighborAdd.set(0);
    Events::neighborRemove.set(0);
	for(unsigned i=0; i<64; ++i) { 
		DoPaint(false);
	}
	//PlayMusic(music_winscreen, false);
	//WinScreen();
}
