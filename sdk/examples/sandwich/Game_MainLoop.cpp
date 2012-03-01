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

	//WinScreen();

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
	      		int progress;
	      		for(progress=0; progress<24; progress+=WALK_SPEED) {
	      			mPlayer.Move(0, -WALK_SPEED);
	      			Paint();
	      		}

	      		if (mPlayer.HasBasicKey()) {
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
	      			do {
	      				Paint();
	      			} while(System::clock() - timeout <  0.5f);
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
	      			mPlayer.TargetView()->HideOverlay(mPlayer.Direction()%2 == 1);
	      		}
	      		bool result = gGame.GetMap()->FindNarrowPath(*mPlayer.Current(), mPlayer.Direction(), &mMoves);
	      		ASSERT(result);
	      		int progress = 0;
	      		uint8_t *pNextMove;
	      		for(pNextMove=mMoves.pFirstMove; pNextMove!=mMoves.End(); ++pNextMove) {
	      			mPlayer.SetDirection(*pNextMove);
	      			if (progress != 0) {
	      				mPlayer.Move(progress * kSideToUnit[*pNextMove]);
	      				Paint();
	      			}
	      			while(progress+WALK_SPEED < 16) {
	      				progress += WALK_SPEED;
	      				mPlayer.Move(WALK_SPEED * kSideToUnit[*pNextMove]);
	      				Paint();
	      			}
	      			mPlayer.Move((16 - progress) * kSideToUnit[*pNextMove]);
	      			progress = WALK_SPEED - (16-progress);
	      		}
	      		if (progress != 0) {
	      			pNextMove--;
	      			mPlayer.Move(progress * kSideToUnit[*pNextMove]);
	      			progress = 0;
	      			Paint();
	      		}
	      	}
		    if (mPlayer.TargetView()) { // did we land on the target?
		    	mPlayer.AdvanceToTarget();
		      	if (OnPassiveTrigger() == TRIGGER_RESULT_PATH_INTERRUPTED) {
		      		mPath.Cancel();
		      	}
		    }
		} while(mPath.PopStep(*mPlayer.Current(), mPlayer.Target()));
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
