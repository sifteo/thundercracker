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
		token->userData = this;
		mStyle = DefaultStyle;
		mLit = false;

		mStatus = StatusIdle;
		mTimeout = -1.0f;
		mDigitId = -1;
		mHideMask = 0;

		//TODO      if (showDigitOnInit) { mDigit.data = mStyle.normalDigits[token.val]; }
		if (!showDigitOnInit) { mDigitId = 0; }
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
		/* TODO
		mDigit.data = mStyle.normalDigits[mDigitId];
		if (mDigit.HasDirtyRect) {
		var aabb = mDigit.DirtyRect;
		Cube.FillRect(new Color(0, 36, 85), aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
		}
		mDigit.Paint(Cube);
		*/
		//Cube.Paint();
	}

	void TokenView::ResetNumeral() 
	{
		if (mDigitId != token->val) {
			/* TODO
			mDigit.data = mStyle.normalDigits[token.val];
			var aabb = mDigit.DirtyRect;
			Cube.FillRect(new Color(0, 36, 85), aabb.position.x, aabb.position.y, aabb.size.x, aabb.size.y);
			mDigit.Paint(Cube);
			*/
		}
		mDigitId = -1;
		PaintBottom(false);
		PaintRight(false);
		// Cube.Paint();
	}

	void TokenView::WillJoinGroup() 
	{
		mCurrentExpression = token->current;
	}

	void TokenView::DidJoinGroup() 
	{
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
			//TODO     mDigit.data = mStyle.accentDigits[token.val];
			for(int i=mCurrentExpression->GetDepth(); i>=0; --i) 
			{
				int mask = 0;
				int connCount = 0;
				for(int j=0; j<4; ++j) {
					Cube::Side s = (Cube::Side)j;
					if (token->ConnectsOnSideAtDepth(s, i, mCurrentExpression))
					{
						//TODO            PaintRect(mStyle.colors[i], s, 16 + 4 * i);
						mask |= (0x1<<j);
						connCount++;
					}
				}
				if (connCount > 0) 
				{
					/*TODO            mCenterCap.data = mStyle.circles[i];
					mCenterCap.Paint(c); */
				}
			}
		}
		else 
		{
			//TODO        mDigit.data = mStyle.normalDigits[token.val];
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
			} else {
			mDigit.Paint(c); */
		} 
	}

	void TokenView::PaintRect(Color color, Cube::Side direction, int halfWidth) 
	{
		if (mStatus == StatusOverlay && (direction == SIDE_LEFT || direction == SIDE_RIGHT))
		{
			return;
		}
		/* TODO
		Int2 p = Int2.Zero;
		Int2 s = Int2.Zero;
		switch(direction) {
		case Cube.Side.TOP:
		p.x = Mid.x - halfWidth;
		s.x = 2 * halfWidth;
		s.y = Mid.y;
		break;
		case Cube.Side.LEFT:
		p.y = Mid.y - halfWidth;
		s.x = Mid.x;
		s.y = 2 * halfWidth;
		break;
		case Cube.Side.BOTTOM:
		p.x = Mid.x - halfWidth;
		p.y = Mid.y;
		s.x = 2 * halfWidth;
		s.y = 128 - p.y;
		break;
		case Cube.Side.RIGHT:
		p.x = Mid.x;
		p.y = Mid.y - halfWidth;
		s.x = 128 - p.x;
		s.y = 2 * halfWidth;
		break;
		}
		Cube.FillRect(color, p.x, p.y, s.x, s.y); */
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
	// // // //
}
