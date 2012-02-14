#include "TokenView.h"
#include "Token.h"
#include "TokenGroup.h"
#include "AudioPlayer.h"
#include "assets.gen.h"

namespace TotalsGame
{
	DEFINE_POOL(TokenView);

	const Vec2 TokenView::Mid(7,7);
	int TokenView::sHintParity = 0;
	const char *TokenView::kOpNames[4] = { "add", "sub", "mul", "div" };


	TokenView::Status TokenView::GetCurrentStatus()
	{
		return mStatus; 
	}

	TokenView::TokenView(TotalsCube *cube, Token *_token, bool showDigitOnInit) 
		: View(cube)
	{
		token = _token;
		mCurrentExpression = token;
        token->view = this;
		mStyle = DefaultStyle;
		mLit = false;

		mStatus = StatusIdle;
		mTimeout = -1.0f;
		mDigitId = -1;
		mHideMask = 0;
        useAccentDigit = false;

        //TODO      normal style
        if (showDigitOnInit)
            mDigitId = token->GetValue().nu;
        else
            mDigitId = 0;

	}

	void TokenView::HideOps() 
	{
		mDigitId = 0;
		Paint();
	}

	void TokenView::PaintRandomNumeral() 
	{
		int d = mDigitId;
		while (d == mDigitId) { mDigitId = Game::rand.randrange(9) + 1; }
        useAccentDigit = false;
        PaintDigit();

		//Cube.Paint();
	}

	void TokenView::ResetNumeral() 
	{
		if (mDigitId != token->val) {
            useAccentDigit = false;
            PaintDigit();
		}
		mDigitId = -1;
		PaintBottom(false);
		PaintRight(false);
		// Cube.Paint();
	}

	void TokenView::WillJoinGroup() 
	{

	}

	void TokenView::DidJoinGroup() 
	{
        mCurrentExpression = token->current;

		TokenGroup *grp = NULL;
		if(token->current->IsTokenGroup())
			grp = (TokenGroup*)token->current;

		mTimeout = Game::rand.uniform(0.2f, 0.25f);
		mStatus = StatusQueued;
		if (grp != NULL) 
		{
			if (grp->GetSrcToken() == token) 
			{
				if (grp->GetSrcSide() == SIDE_RIGHT)
				{
					PaintRight(true);
				} 
				else 
				{
					PaintBottom(true);
				}
				//     Cube.Paint();
			} 
			else if (grp->GetDstToken() == token)
			{
				if (grp->GetSrcSide() == SIDE_RIGHT) 
				{
					PaintLeft(true);
				} 
				else
				{
					PaintTop(true);
				}
				//Cube.Paint();
			}
		}
	}

	void TokenView::DidGroupDisconnect() 
	{
		SetState(StatusIdle);
	}

	void TokenView::BlinkOverlay()
	{
		SetState(StatusOverlay);
		mTimeout = 2;
	}

	void TokenView::ShowOverlay() 
	{
		SetState(StatusOverlay);
		mTimeout = -1.0f;
	}

	void TokenView::ShowLit() 
	{
		if (!mLit) {
			mLit = true;
			Paint();
		}
	}

	void TokenView::HideOverlay()
	{
		if (mStatus == StatusOverlay) 
		{
			SetState(StatusIdle);
		}
	}

	void TokenView::SetHideMode(int mask) 
	{
		if (mHideMask != mask) 
		{
			mHideMask = mask;
			Paint();
		}
	}

	bool TokenView::NotHiding(Cube::Side side) 
	{ 
		return (mHideMask & (1<<(int)side)) == 0; 
	}

	void TokenView::SetState(Status state, bool resetTimer, bool resetExpr)
	{
		if (mStatus != state || (resetExpr && mCurrentExpression != token->current)) 
		{
			if (mStatus == StatusHinting) 
			{ 
				AudioPlayer::PlaySfx(sfx_Hide_Overlay); 
			}
			mStatus = state;
			if (mStatus == StatusHinting) 
			{
				sHintParity = 1 - sHintParity;
				if(sHintParity)
					AudioPlayer::PlaySfx(sfx_Hint_Stinger_01);
				else
					AudioPlayer::PlaySfx(sfx_Hint_Stinger_02);
			}
			if (resetTimer) { mTimeout = -1.0f; }
			if (resetExpr) { mCurrentExpression = token->current; }
			Paint();
		}
	}

	void TokenView::DidAttachToCube(TotalsCube *c) 
	{
        c->AddEventHandler(&eventHandler);
	}  

	void TokenView::OnShakeStarted(TotalsCube *c) 
	{		
		if (token->GetPuzzle()->target == NULL || token->current != token) { return; }

		sHintParity = 1 - sHintParity;
		if (!token->GetPuzzle()->unlimitedHints && token->GetPuzzle()->hintsUsed >= token->GetPuzzle()->GetNumTokens()-1) 
		{
			if(sHintParity)
				AudioPlayer::PlaySfx(sfx_Hint_Deny_01);
			else
				AudioPlayer::PlaySfx(sfx_Hint_Deny_02);
		} 
		else 
		{
			token->GetPuzzle()->hintsUsed++;
			mCurrentExpression = token->GetPuzzle()->target;
			mTimeout = 2.0f;
			SetState(StatusHinting, false, false);
		}
	}

	void TokenView::OnButtonEvent(TotalsCube *c, bool isPressed) {
		if (!isPressed && token->GetPuzzle()->target != NULL)
		{
			BlinkOverlay();
			AudioPlayer::PlaySfx(sfx_Target_Overlay);
		}
	}

	void TokenView::WillDetachFromCube(TotalsCube *c) 
	{
        c->RemoveEventHandler(&eventHandler);
	}

	void TokenView::Update(float dt) 
	{
		if (mTimeout >= 0) 
		{
			mTimeout -= dt;
			if (mTimeout < 0 && mStatus != StatusIdle)
			{
				if (mStatus == StatusQueued) 
				{

					TokenGroup *grp = NULL;
					if(token->current->IsTokenGroup())
						grp = (TokenGroup*)token->current;
					if (grp != NULL && (token == grp->GetSrcToken() || token == grp->GetDstToken())) 
					{
						BlinkOverlay();
					} 
					else 
					{
						SetState(StatusIdle);
					}
				} 
				else
				{
					if (mStatus == StatusOverlay) {
						AudioPlayer::PlaySfx(sfx_Hide_Overlay);
					}
					SetState(StatusIdle);
				}
			}
		}
	}

	void TokenView::Paint() 
	{
		TotalsCube *c = GetCube();
		c->Image(mLit ? &BackgroundLit : &Background);
		SideStatus bottomStatus = SideStatusOpen;
		SideStatus rightStatus = SideStatusOpen;
		SideStatus topStatus = SideStatusOpen;
		SideStatus leftStatus = SideStatusOpen;
		if (mDigitId >= 0) 
		{
			// suppress operator image?
			PaintTop(false);
			PaintBottom(false);
			PaintLeft(false);
			PaintRight(false);
		} 
		else
		{
			bottomStatus = token->StatusOfSide(SIDE_BOTTOM, mCurrentExpression);
			rightStatus = token->StatusOfSide(SIDE_RIGHT, mCurrentExpression);
			topStatus = token->StatusOfSide(SIDE_TOP, mCurrentExpression);
			leftStatus = token->StatusOfSide(SIDE_LEFT, mCurrentExpression);
			if (token->current->GetCount() < token->GetPuzzle()->GetNumTokens()) 
			{
				if (NotHiding(SIDE_TOP) && topStatus == SideStatusOpen) { PaintTop(false); }
				if (NotHiding(SIDE_LEFT) && leftStatus == SideStatusOpen) { PaintLeft(false); }
				if (NotHiding(SIDE_BOTTOM) && bottomStatus == SideStatusOpen) { PaintBottom(false); }
				if (NotHiding(SIDE_RIGHT) && rightStatus == SideStatusOpen) { PaintRight(false); }
			}
		}


        bool isInGroup = mCurrentExpression != token;
        if (isInGroup)
        {
            uint8_t masks[4] = {0};
            useAccentDigit = true;
            for(int i=mCurrentExpression->GetDepth(); i>=0; --i)
            {
                int connCount = 0;
                for(int j=0; j<NUM_SIDES; ++j)
                {
                    if (token->ConnectsOnSideAtDepth(j, i, mCurrentExpression))
                    {
                        masks[j] |= 1 << i;
                        connCount++;
                    }
                }
            }

            PaintCenterCap(masks);
        }


        else
        {
            useAccentDigit = false;
        }
		if (mDigitId == -1) 
		{
			if (NotHiding(SIDE_TOP) && topStatus == SideStatusConnected) { PaintTop(true); }
			if (NotHiding(SIDE_LEFT) && leftStatus == SideStatusConnected) { PaintLeft(true); }
			if (NotHiding(SIDE_BOTTOM) && bottomStatus == SideStatusConnected) { PaintBottom(true); }
			if (NotHiding(SIDE_RIGHT) && rightStatus == SideStatusConnected) { PaintRight(true); }
		}
		if (mStatus == StatusOverlay)
		{
			Fraction result = token->GetCurrentTotal();
			const int yPos = 24;
			//TODO        c->Image("accent", 0, yPos, 0, 0, 128, 64);
			/*        var img = Game.Inst.Images[isInGroup ? "accent_current" : "accent_target"];
			c.Image(img.name, (128-img.width)>>1, yPos+8);
			if (result.IsNAN) {
			var nanData = Game.Inst.Images["nan"];
			c.Image("nan", (128-nanData.width)>>1, yPos+32-nanData.height/2);
			} else {
			c.DrawFraction(result, 64, yPos+32);
			}
            } else {*/
            //PaintDigit();
		} 
        PaintDigit();
	}

	// // // //
	// code generated by ImageTool
	void TokenView::PaintTop(bool lit) {
		/*TODO     if (lit) {
		Cube.Image("lit_top", 46, 0, 0, 0, 22, 10);
		} else {
		Cube.Image("unlit_top", 46, 0, 0, 0, 28, 12);
		} */
	}
	void TokenView::PaintLeft(bool lit) {
		/*TODO      if (mStatus == Status.Overlay) { return; }
		if (lit) {
		Cube.Image("lit_left", 0, 46, 0, 0, 8, 22);
		} else {
		Cube.Image("unlit_left", 0, 45, 0, 0, 12, 25);
		}*/
	}
	void TokenView::PaintRight(bool lit) {
		/*TODO      if (mStatus == Status.Overlay) { return; }
		if (mDigitId >= 0) {
		if (lit) {
		Cube.Image("lit_right", 106, 44, 0, 0, 22, 24);
		} else {
		Cube.Image("unlit_right", 106, 44, 0, 0, 22, 27);
		}
		} else {
		var name = kOpNames[(int)token.OpRight];
		if (lit) {
		Cube.Image("lit_right_"+name, 106, 44, 0, 0, 22, 24);
		} else {
		Cube.Image("unlit_right_"+name, 106, 44, 0, 0, 22, 27);
		}
		} */
	}
	void TokenView::PaintBottom(bool lit) {
		/*TODO     if (mDigitId >= 0) {
		if (lit) {
		Cube.Image("lit_bottom", 45, 106, 0, 0, 24, 22);
		} else {
		Cube.Image("unlit_bottom", 45, 106, 0, 0, 29, 22);
		}
		} else {
		var name = kOpNames[(int)token.OpBottom];
		if (lit) {
		Cube.Image("lit_bottom_"+name, 45, 106, 0, 0, 24, 22);
		} else {
		Cube.Image("unlit_bottom_"+name, 45, 106, 0, 0, 29, 22);
		}
		}*/
	}
    
    void TokenView::PaintCenterCap(uint8_t masks[4])
    {
	    //uint8_t masks[4] = {15,15,15,15};
	    
        // compute the screen state union (assuming it's valid)
        uint8_t vunion = masks[0] | masks[1] | masks[2] | masks[3];
        TotalsCube *c = GetCube();
        
        ASSERT(vunion < (1<<6));
        
        // flood fill the background to start (not optimal, but this is a demo, dogg)
        VidMode_BG0_SPR_BG1 mode(c->vbuf);
        mode.BG0_drawAsset(Vec2(0,0), Background);
        if (vunion == 0x00) { return; }
        
        // determine the "lowest" bit
        unsigned lowestBit = 0;
        while(!(vunion & (1<<lowestBit))) { lowestBit++; }
        
        // Center
        for(unsigned i=5; i<9; ++i) {
            mode.BG0_drawAsset(Vec2(i,4), Center, lowestBit);
            mode.BG0_drawAsset(Vec2(i,9), Center, lowestBit);
        }
        for(unsigned i=5; i<9; ++i)
            for(unsigned j=4; j<10; ++j) {
                mode.BG0_drawAsset(Vec2(j,i), Center, lowestBit);
            }
        
        // Horizontal
        mode.BG0_drawAsset(Vec2(3,0), Horizontal, masks[SIDE_TOP]);
        mode.BG0_drawAsset(Vec2(3,13), Horizontal, masks[SIDE_BOTTOM]);
        mode.BG0_drawAsset(Vec2(3,14), Horizontal, masks[SIDE_BOTTOM]);
        mode.BG0_drawAsset(Vec2(3,15), Horizontal, masks[SIDE_BOTTOM]);
        
        // Vertical
        mode.BG0_drawAsset(Vec2(0,3), Vertical, masks[SIDE_LEFT]);
        mode.BG0_drawAsset(Vec2(13,3), Vertical, masks[SIDE_RIGHT]);
        mode.BG0_drawAsset(Vec2(14,3), Vertical, masks[SIDE_RIGHT]);
        mode.BG0_drawAsset(Vec2(15,3), Vertical, masks[SIDE_RIGHT]);
        
        // Minor Diagonals
        if (vunion & 0x10) {
            mode.BG0_drawAsset(Vec2(2,2), MinorNW, 1);
            mode.BG0_drawAsset(Vec2(2,11), MinorSW, 1);
            mode.BG0_drawAsset(Vec2(11,11), MinorSE, 0);
            mode.BG0_drawAsset(Vec2(11,2), MinorNE, 0);
        }
        
        // Major Diagonals
        if (lowestBit > 1) {
            mode.BG0_drawAsset(Vec2(4,4), Center, lowestBit);
            mode.BG0_drawAsset(Vec2(4,9), Center, lowestBit);
            mode.BG0_drawAsset(Vec2(9,9), Center, lowestBit);
            mode.BG0_drawAsset(Vec2(9,4), Center, lowestBit);
        } else {
            mode.BG0_drawAsset(Vec2(4,4), MajorNW, vunion & 0x03);
            mode.BG0_drawAsset(Vec2(4,9), MajorSW, vunion & 0x03);
            mode.BG0_drawAsset(Vec2(9,9), MajorSE, 3 - (vunion & 0x03));
            mode.BG0_drawAsset(Vec2(9,4), MajorNE, 3 - (vunion & 0x03));
        }
        
        static const uint8_t keyIndices[] = { 0, 1, 3, 5, 8, 10, 13, 16, 20, 22, 25, 28, 32, 35, 39, 43, 48, 50, 53, 56, 60, 63, 67, 71, 76, 79, 83, 87, 92, 96, 101, 106, };
        
        // Major Joints
        mode.BG0_drawAsset(Vec2(3,1), MajorN, keyIndices[vunion] + CountBits(vunion ^ masks[0]));
        mode.BG0_drawAsset(Vec2(1,3), MajorW, keyIndices[vunion] + CountBits(vunion ^ masks[1]));
        mode.BG0_drawAsset(Vec2(3,10), MajorS, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[2]));
        mode.BG0_drawAsset(Vec2(10,3), MajorE, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[3]));
    }
    
    
    int TokenView::CountBits(uint8_t mask) {
        unsigned result = 0;
        for(unsigned i=0; i<5; ++i) {
            if (mask & (1<<i)) { result++; }
        }
        return result;
    }
    

    void TokenView::PaintDigit()
    {
        static const AssetImage * const assets[20] =
        {
            &NormalDigit_0,
            &NormalDigit_1,
            &NormalDigit_2,
            &NormalDigit_3,
            &NormalDigit_4,
            &NormalDigit_5,
            &NormalDigit_6,
            &NormalDigit_7,
            &NormalDigit_8,
            &NormalDigit_9,
            &AccentDigit_0,
            &AccentDigit_1,
            &AccentDigit_2,
            &AccentDigit_3,
            &AccentDigit_4,
            &AccentDigit_5,
            &AccentDigit_6,
            &AccentDigit_7,
            &AccentDigit_8,
            &AccentDigit_9,

        };

        BG1Helper bg1(*GetCube());

        if(mDigitId >=0 && mDigitId <= 9)
        {
            Vec2 p;
            p.x = TokenView::Mid.x - assets[mDigitId]->width / 2;
            p.y = TokenView::Mid.y - assets[mDigitId]->height / 2;
            bg1.DrawAsset(p, *assets[useAccentDigit? mDigitId + 10 : mDigitId]);
        }
        bg1.Flush();

    }

    
	// // // //
}










