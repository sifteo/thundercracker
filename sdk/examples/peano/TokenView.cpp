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
        : View(cube), eventHandler(this)
	{
        /* warning: big hack TODO
          view constructor calls virtual DidAttachToCube
          since we're currently in a constructor the virtual call
          goes to the base class, not *this* class.
          I'll just call it manually to make it work.
          */
        TokenView::DidAttachToCube(cube);

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
        if (renderedDigit != token->val) {
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
        c->Image(mLit ? &BackgroundLit : &Background, Vec2(0,0));
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
            const int yPos = 3;
            //TODO
            c->Image(&Accent, Vec2(0,yPos));
            if(isInGroup)
            {
                c->foregroundLayer.DrawAsset(Vec2(8-Accent_Current.width/2, yPos+1), Accent_Current);
            }
            else
            {
                c->foregroundLayer.DrawAsset(Vec2(8-Accent_Target.width/2, yPos+1), Accent_Target);
            }

            if (result.IsNan())
            {
                c->foregroundLayer.DrawAsset(Vec2(8-Nan.width/2, yPos+4-Nan.height/2), Nan);
			} else {
                //uses sprites -> pixel coords
                c->DrawFraction(result, Vec2(64, yPos*8+32));
            }
		} 
        else
        {
            PaintDigit();
        }
	}

	// // // //
	// code generated by ImageTool
	void TokenView::PaintTop(bool lit) {

        if (lit) {
            GetCube()->foregroundLayer.DrawAsset(Vec2(Mid.x - Lit_Top.width/2, 0), Lit_Top);
		} else {
        GetCube()->foregroundLayer.DrawAsset(Vec2(Mid.x - Unlit_Top.width/2, 0), Unlit_Top);
        }
	}
	void TokenView::PaintLeft(bool lit) {
        if (mStatus == StatusOverlay) { return; }
        if (lit) {
        GetCube()->foregroundLayer.DrawAsset(Vec2(0, Mid.y - Lit_Left.height/2), Lit_Left);
        } else {
        GetCube()->foregroundLayer.DrawAsset(Vec2(0, Mid.y - Unlit_Left.height/2), Unlit_Left);
        }
    }
	void TokenView::PaintRight(bool lit) {
        if (mStatus == StatusOverlay) { return; }
        if (mDigitId >= 0) {
            if (lit) {
            GetCube()->foregroundLayer.DrawAsset(Vec2(16-Lit_Right.width, Mid.y - Lit_Right.height/2), Lit_Right);
            } else {
            GetCube()->foregroundLayer.DrawAsset(Vec2(16-Unlit_Right.width, Mid.y - Unlit_Right.height/2), Unlit_Right);
            }
        } else {
            if (lit) {
                static const AssetImage* const assets[4]=
                {
                    &Lit_Right_Add,
                    &Lit_Right_Sub,
                    &Lit_Right_Mul,
                    &Lit_Right_Div,
                };
                const AssetImage *asset = assets[(int)token->GetOpRight()];
                GetCube()->foregroundLayer.DrawAsset(Vec2(16-asset->width, Mid.y - asset->height/2), *asset);
            } else {
                static const AssetImage* const assets[4]=
                {
                    &Unlit_Right_Add,
                    &Unlit_Right_Sub,
                    &Unlit_Right_Mul,
                    &Unlit_Right_Div,
                };
                const AssetImage *asset = assets[(int)token->GetOpRight()];
                GetCube()->foregroundLayer.DrawAsset(Vec2(16-asset->width, Mid.y - asset->height/2), *asset);
            }

        }
	}
	void TokenView::PaintBottom(bool lit) {
        if (mDigitId >= 0) {
            if (lit) {
            GetCube()->foregroundLayer.DrawAsset(Vec2(Mid.x - Lit_Bottom.width/2, 16-Lit_Bottom.height), Lit_Bottom);
            } else {
            GetCube()->foregroundLayer.DrawAsset(Vec2(Mid.x - Unlit_Bottom.width/2, 16-Lit_Bottom.height), Unlit_Bottom);
            }
		} else {
            if (lit) {
                static const AssetImage* const assets[4] =
                {
                    &Lit_Bottom_Add,
                    &Lit_Bottom_Sub,
                    &Lit_Bottom_Mul,
                    &Lit_Bottom_Div,
                };
                const AssetImage *asset = assets[(int)token->GetOpBottom()];
                GetCube()->foregroundLayer.DrawAsset(Vec2(Mid.x - asset->width/2, 16-asset->height), *asset);
            } else {
                static const AssetImage* const assets[4]=
                {
                    &Unlit_Bottom_Add,
                    &Unlit_Bottom_Sub,
                    &Unlit_Bottom_Mul,
                    &Unlit_Bottom_Div,
                };
                const AssetImage* asset = assets[(int)token->GetOpBottom()];
                GetCube()->foregroundLayer.DrawAsset(Vec2(Mid.x - asset->width/2, 16-asset->height), *asset);
            }

        }
	}
    
    void TokenView::PaintCenterCap(uint8_t masks[4])
    {
	    //uint8_t masks[4] = {15,15,15,15};
	    
        // compute the screen state union (assuming it's valid)
        uint8_t vunion = masks[0] | masks[1] | masks[2] | masks[3];
        TotalsCube *c = GetCube();
        
        ASSERT(vunion < (1<<6));
        
        // flood fill the background to start (not optimal, but this is a demo, dogg)
        GetCube()->backgroundLayer.BG0_drawAsset(Vec2(0,0), Background);
        if (vunion == 0x00) { return; }
        
        // determine the "lowest" bit
        unsigned lowestBit = 0;
        while(!(vunion & (1<<lowestBit))) { lowestBit++; }
        
        // Center
        for(unsigned i=5; i<9; ++i) {
            GetCube()->backgroundLayer.BG0_drawAsset(Vec2(i,4), Center, lowestBit);
            GetCube()->backgroundLayer.BG0_drawAsset(Vec2(i,9), Center, lowestBit);
        }
        for(unsigned i=5; i<9; ++i)
            for(unsigned j=4; j<10; ++j) {
                GetCube()->backgroundLayer.BG0_drawAsset(Vec2(j,i), Center, lowestBit);
            }
        
        // Horizontal
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(3,0), Horizontal, masks[SIDE_TOP]);
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(3,13), Horizontal, masks[SIDE_BOTTOM]);
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(3,14), Horizontal, masks[SIDE_BOTTOM]);
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(3,15), Horizontal, masks[SIDE_BOTTOM]);
        
        // Vertical
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(0,3), Vertical, masks[SIDE_LEFT]);
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(13,3), Vertical, masks[SIDE_RIGHT]);
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(14,3), Vertical, masks[SIDE_RIGHT]);
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(15,3), Vertical, masks[SIDE_RIGHT]);
        
        // Minor Diagonals
        if (vunion & 0x10) {
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(2,2), MinorNW, 1);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(2,11), MinorSW, 1);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(11,11), MinorSE, 0);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(11,2), MinorNE, 0);
        }
        
        // Major Diagonals
        if (lowestBit > 1) {
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(4,4), Center, lowestBit);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(4,9), Center, lowestBit);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(9,9), Center, lowestBit);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(9,4), Center, lowestBit);
        } else {
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(4,4), MajorNW, vunion & 0x03);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(4,9), MajorSW, vunion & 0x03);
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(9,9), MajorSE, 3 - (vunion & 0x03));
             GetCube()->backgroundLayer.BG0_drawAsset(Vec2(9,4), MajorNE, 3 - (vunion & 0x03));
        }
        
        static const uint8_t keyIndices[] = { 0, 1, 3, 5, 8, 10, 13, 16, 20, 22, 25, 28, 32, 35, 39, 43, 48, 50, 53, 56, 60, 63, 67, 71, 76, 79, 83, 87, 92, 96, 101, 106, };
        
        // Major Joints
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(3,1), MajorN, keyIndices[vunion] + CountBits(vunion ^ masks[0]));
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(1,3), MajorW, keyIndices[vunion] + CountBits(vunion ^ masks[1]));
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(3,10), MajorS, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[2]));
         GetCube()->backgroundLayer.BG0_drawAsset(Vec2(10,3), MajorE, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[3]));
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

        int digit = token->val;
        if(digit >=0 && digit <= 9)
        {
            int index = useAccentDigit? digit + 10 : digit;
            Vec2 p;
            p.x = TokenView::Mid.x - assets[index]->width / 2;
            p.y = TokenView::Mid.y - assets[index]->height / 2;
            GetCube()->foregroundLayer.DrawAsset(p, *assets[index]);
        }

    }


    
	// // // //
}










