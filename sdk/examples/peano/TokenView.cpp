#include "TokenView.h"
#include "Token.h"
#include "TokenGroup.h"
#include "AudioPlayer.h"
#include "assets.gen.h"
#include "Skins.h"

namespace TotalsGame
{
const Vec2 TokenView::Mid(7,7);
int TokenView::sHintParity = 0;
const char *TokenView::kOpNames[4] = { "add", "sub", "mul", "div" };


TokenView::Status TokenView::GetCurrentStatus()
{
    return mStatus;
}

TokenView::TokenView()
    : eventHandler(this)
{
    Reset();
}

void TokenView::Reset()
{
    mCurrentExpression = NULL;
    mLit = false;

    mStatus = StatusIdle;
    mTimeout = -1.0f;
    mHideMask = 0;
    mDigitId = 0;
    useAccentDigit = false;
}

void TokenView::SetToken(Token *t)
{
    token = t;
    token->view = this;
    renderedDigit = token->val;
    mCurrentExpression = token;
    mDigitId = -1;
}

void TokenView::HideOps()
{
    mDigitId = 0;
    PaintNow();
}

void TokenView::PaintRandomNumeral()
{
    int d = mDigitId;
    while (d == mDigitId) { mDigitId = Game::rand.randrange(9) + 1; }
    useAccentDigit = false;
    renderedDigit = mDigitId;
    PaintNow();
}

void TokenView::ResetNumeral()
{
    if (renderedDigit != token->val) {
        useAccentDigit = false;
        renderedDigit = token->val;
    }
    mDigitId = -1;
    PaintNow();
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
    //disable any shown sprites
    GetCube()->HideSprites();

    if (grp != NULL)
    {
        PaintNow();
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
        PaintNow();
    }
}

void TokenView::HideOverlay()
{
    if (mStatus == StatusOverlay)
    {
        SetState(StatusIdle);
        PaintNow();
    }
}

void TokenView::SetHideMode(int mask)
{
    if (mHideMask != mask)
    {
        mHideMask = mask;
        PaintNow();
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
            PLAY_SFX(sfx_Hide_Overlay);
        }
        mStatus = state;
        if (mStatus == StatusHinting)
        {
            sHintParity = 1 - sHintParity;
            if(sHintParity)
                PLAY_SFX(sfx_Hint_Stinger_01);
            else
                PLAY_SFX(sfx_Hint_Stinger_02);
        }
        if (resetTimer) { mTimeout = -1.0f; }
        if (resetExpr) { mCurrentExpression = token->current; }

        if(state != StatusOverlay)
        {
            //hide the overlay sprites that were drawn by the overlay
            TotalsCube *cube = GetCube();
            if(cube)
            {
                GetCube()->HideSprites();
            }
        }
        PaintNow();
    }
}

void TokenView::DidAttachToCube(TotalsCube *c)
{
    c->AddEventHandler(&eventHandler);
}

void TokenView::WillDetachFromCube(TotalsCube *c)
{
    c->RemoveEventHandler(&eventHandler);
}

void TokenView::OnShakeStarted(TotalsCube *c)
{
    if (token->GetPuzzle()->target == NULL || token->current != token) { return; }

    sHintParity = 1 - sHintParity;
    if (!token->GetPuzzle()->unlimitedHints && token->GetPuzzle()->hintsUsed >= token->GetPuzzle()->GetNumTokens()-1)
    {
        if(sHintParity)
            PLAY_SFX(sfx_Hint_Deny_01);
        else
            PLAY_SFX(sfx_Hint_Deny_02);
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
        PLAY_SFX(sfx_Target_Overlay);
    }
}

void TokenView::Update()
{
    if (mTimeout >= 0)
    {
        mTimeout -= Game::dt;
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
                    PLAY_SFX(sfx_Hide_Overlay);
                }
                SetState(StatusIdle);
            }
        }
    }
}

void TokenView::PaintNow()
{
    TotalsCube *c = GetCube();
    if(!c) return;

    c->foregroundLayer.Clear();
    c->HideSprites();

    const Skins::Skin &skin = Skins::GetSkin();

    c->Image(mLit ? skin.background_lit : skin.background, Vec2(0,0));
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
        renderedDigit = token->val;
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
        //used to set sprite.  moved into mDigitId !=- 1 just below
        //so random numeral can render
        useAccentDigit = false;
    }
    if (mDigitId == -1)
    {
        renderedDigit = token->val;

        if (NotHiding(SIDE_TOP) && topStatus == SideStatusConnected) { PaintTop(true); }
        if (NotHiding(SIDE_LEFT) && leftStatus == SideStatusConnected) { PaintLeft(true); }
        if (NotHiding(SIDE_BOTTOM) && bottomStatus == SideStatusConnected) { PaintBottom(true); }
        if (NotHiding(SIDE_RIGHT) && rightStatus == SideStatusConnected) { PaintRight(true); }
    }
    if (mStatus == StatusOverlay)
    {
        Fraction result = token->GetCurrentTotal();
        const int yPos = 3;
        c->Image(skin.accent, Vec2(0,yPos));
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

    c->foregroundLayer.Flush();
}

// // // //
// code generated by ImageTool
//top and bottom are on high # sprites because they show when text is also draw
//during status overlay, which also uses sprites at low #.
void TokenView::PaintTop(bool lit) {
    const Skins::Skin &skin = Skins::GetSkin();
    const PinnedAssetImage &asset = lit? skin.lit_top : skin.unlit_top;
    GetCube()->backgroundLayer.setSpriteImage(6, asset, 0);
    GetCube()->backgroundLayer.moveSprite(6, 8*Vec2(Mid.x - asset.width/2, 0));
}
void TokenView::PaintLeft(bool lit) {
    if (mStatus == StatusOverlay) { return; }
    const Skins::Skin &skin = Skins::GetSkin();
    const PinnedAssetImage &asset = lit? skin.lit_left : skin.unlit_left;
    GetCube()->backgroundLayer.setSpriteImage(1, asset, 0);
    GetCube()->backgroundLayer.moveSprite(1, 8*Vec2(0, Mid.y - asset.height/2));
}
void TokenView::PaintRight(bool lit) {
    if (mStatus == StatusOverlay) { return; }
    const Skins::Skin &skin = Skins::GetSkin();
    if (mDigitId >= 0) {
        const PinnedAssetImage &asset = lit? skin.lit_right : skin.unlit_right;
        GetCube()->backgroundLayer.setSpriteImage(2, asset, 0);
        GetCube()->backgroundLayer.moveSprite(2, 8*Vec2(16-asset.width, Mid.y - asset.height/2));
    } else {
        if (lit) {
            const PinnedAssetImage* const assets[4]=
            {
                &skin.lit_right_add,
                &skin.lit_right_sub,
                &skin.lit_right_mul,
                &skin.lit_right_div,
            };
            const PinnedAssetImage *asset = assets[(int)token->GetOpRight()];
            GetCube()->backgroundLayer.setSpriteImage(2, *asset, 0);
            GetCube()->backgroundLayer.moveSprite(2, 8*Vec2(16-asset->width, Mid.y - asset->height/2));
        } else {
            const PinnedAssetImage* const assets[4]=
            {
                &skin.unlit_right_add,
                &skin.unlit_right_sub,
                &skin.unlit_right_mul,
                &skin.unlit_right_div,
            };
            const PinnedAssetImage *asset = assets[(int)token->GetOpRight()];
            GetCube()->backgroundLayer.setSpriteImage(2, *asset, 0);
            GetCube()->backgroundLayer.moveSprite(2, 8*Vec2(16-asset->width, Mid.y - asset->height/2));
        }

    }
}
void TokenView::PaintBottom(bool lit)
{
    const Skins::Skin &skin = Skins::GetSkin();
    if (mDigitId >= 0) {
        const PinnedAssetImage &asset = lit? skin.lit_bottom : skin.unlit_bottom;
        GetCube()->backgroundLayer.setSpriteImage(7, asset, 0);
        GetCube()->backgroundLayer.moveSprite(7, 8*Vec2(Mid.x - asset.width/2, 16-asset.height));
    } else {
        if (lit) {
            const PinnedAssetImage* const assets[4] =
            {
                &skin.lit_bottom_add,
                &skin.lit_bottom_sub,
                &skin.lit_bottom_mul,
                &skin.lit_bottom_div,
            };
            const PinnedAssetImage *asset = assets[(int)token->GetOpBottom()];
            GetCube()->backgroundLayer.setSpriteImage(7, *asset, 0);
            GetCube()->backgroundLayer.moveSprite(7, 8*Vec2(Mid.x - asset->width/2, 16-asset->height));
        } else {
            const PinnedAssetImage* const assets[4]=
            {
                &skin.unlit_bottom_add,
                &skin.unlit_bottom_sub,
                &skin.unlit_bottom_mul,
                &skin.unlit_bottom_div,
            };
            const PinnedAssetImage* asset = assets[(int)token->GetOpBottom()];
            GetCube()->backgroundLayer.setSpriteImage(7, *asset, 0);
            GetCube()->backgroundLayer.moveSprite(7, 8*Vec2(Mid.x - asset->width/2, 16-asset->height));
        }

    }
}

void TokenView::PaintCenterCap(uint8_t masks[4])
{
    // compute the screen state union (assuming it's valid)
    uint8_t vunion = masks[0] | masks[1] | masks[2] | masks[3];
    TotalsCube *c = GetCube();

    ASSERT(vunion < (1<<6));

    if (vunion == 0x00) { return; }

    // determine the "lowest" bit
    unsigned lowestBit = 0;
    while(!(vunion & (1<<lowestBit))) { lowestBit++; }

    bool showingOverlay = mStatus == StatusOverlay;

    // Center
    if(lowestBit < 5 && !showingOverlay)   //5 is completely transparent.  the rest, completely opaque
    {
        for(unsigned i=5; i<9; ++i) {
            c->backgroundLayer.BG0_drawAsset(Vec2(i,4), Center, lowestBit);
            c->backgroundLayer.BG0_drawAsset(Vec2(i,9), Center, lowestBit);
        }
        for(unsigned i=5; i<9; ++i) {
            for(unsigned j=4; j<10; ++j) {
                c->backgroundLayer.BG0_drawAsset(Vec2(j,i), Center, lowestBit);
            }
        }
    }

    // Horizontal
#define HORIZONTAL_AT(x, y, side)    do {\
     c->foregroundLayer.DrawAsset(Vec2(x,y), Horizontal_Left, masks[side]); \
    if(masks[side]) {\
        c->backgroundLayer.BG0_drawAsset(Vec2(x+2,y), Horizontal_Center, masks[side]); \
    }\
    c->foregroundLayer.DrawAsset(Vec2(x+6,y), Horizontal_Right, masks[side]); }while(0)


    HORIZONTAL_AT(3,0,SIDE_TOP);
    HORIZONTAL_AT(3,13, SIDE_BOTTOM);
    HORIZONTAL_AT(3,14, SIDE_BOTTOM);
    HORIZONTAL_AT(3,15, SIDE_BOTTOM);
#undef HORIZONTAL_AT

#define VERTICTAL_AT(x, y, side)    do {\
     if(!showingOverlay) c->foregroundLayer.DrawAsset(Vec2(x,y), Vertical_Top, masks[side]); \
    if(masks[side]) c->backgroundLayer.BG0_drawAsset(Vec2(x,y+2), Vertical_Center, masks[side]); \
    if(!showingOverlay) c->foregroundLayer.DrawAsset(Vec2(x,y+6), Vertical_Bottom, masks[side]); }while(0)

    // Vertical
    VERTICTAL_AT(0,3,SIDE_LEFT);
    VERTICTAL_AT(13,3,SIDE_RIGHT);
    VERTICTAL_AT(14,3,SIDE_RIGHT);
    VERTICTAL_AT(15,3,SIDE_RIGHT);
#undef VERTICTAL_AT

    // Minor Diagonals
    if (vunion & 0x10) {
        c->foregroundLayer.DrawAsset(Vec2(2,2), MinorNW, 1);
        c->foregroundLayer.DrawAsset(Vec2(2,11), MinorSW, 1);
        c->foregroundLayer.DrawAsset(Vec2(11,11), MinorSE, 0);
        c->foregroundLayer.DrawAsset(Vec2(11,2), MinorNE, 0);
    }

    // Major Diagonals
    if(!showingOverlay)
    {
        if (lowestBit > 1) {
            c->backgroundLayer.BG0_drawAsset(Vec2(4,4), Center, lowestBit);
            c->backgroundLayer.BG0_drawAsset(Vec2(4,9), Center, lowestBit);
            c->backgroundLayer.BG0_drawAsset(Vec2(9,9), Center, lowestBit);
            c->backgroundLayer.BG0_drawAsset(Vec2(9,4), Center, lowestBit);
        } else {
            c->foregroundLayer.DrawAsset(Vec2(4,4), MajorNW, vunion & 0x03);
            c->foregroundLayer.DrawAsset(Vec2(4,9), MajorSW, vunion & 0x03);
            c->foregroundLayer.DrawAsset(Vec2(9,9), MajorSE, 3 - (vunion & 0x03));
            c->foregroundLayer.DrawAsset(Vec2(9,4), MajorNE, 3 - (vunion & 0x03));
        }
    }

    static const uint8_t keyIndices[] = { 0, 1, 3, 5, 8, 10, 13, 16, 20, 22, 25, 28, 32, 35, 39, 43, 48, 50, 53, 56, 60, 63, 67, 71, 76, 79, 83, 87, 92, 96, 101, 106, };

    // Major Joints            
    if(!showingOverlay)
    {
        c->foregroundLayer.DrawAsset(Vec2(3,1), MajorN, keyIndices[vunion] + CountBits(vunion ^ masks[0]));
        c->foregroundLayer.DrawAsset(Vec2(1,3), MajorW, keyIndices[vunion] + CountBits(vunion ^ masks[1]));
        c->foregroundLayer.DrawAsset(Vec2(3,10), MajorS, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[2]));
        c->foregroundLayer.DrawAsset(Vec2(10,3), MajorE, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[3]));

        //small hack to keep foreground tile count under 144.
        //a few numbers are slightly obscured at the top and bottom
        //by all this stuff in the foreground.
        //for most frames, the center 4 tiles at the bottom/top of north/south
        //can safely be put in the background.
        //when needed, put a hack tile in the foreground to show the few
        //pixels of the number that were obscured.
        //the first and last frame of north/south are completely transparent
        //so dont put them on the background.

        //in case of certain digits
        //draw center 4 of north/south, not frame 0/111 to backbuffer
        //on frame 2/109, hack to frontbuffer
        if(renderedDigit == 2 || renderedDigit == 3 || renderedDigit == 5 || renderedDigit == 6 ||
                renderedDigit == 7 || renderedDigit == 8 || renderedDigit == 9 ||
                renderedDigit == 0)
        {   //top
            int frame = keyIndices[vunion] + CountBits(vunion ^ masks[0]);
            if(0 != frame && 2 != frame)
            {
                c->backgroundLayer.BG0_drawPartialAsset(Vec2(5,3), Vec2(2,2), Vec2(4,1), MajorN, frame);
                c->foregroundLayer.DrawAsset(Vec2(5,3), Transparent_4x1);
            }

            if(2 == frame)
                c->foregroundLayer.DrawAsset(Vec2(3,3), MajorN_Frame2_AccentDigit);
        }

        if(renderedDigit == 2 || renderedDigit == 3 || renderedDigit == 5 ||
                renderedDigit == 6 || renderedDigit == 8 || renderedDigit == 0)
        {   //bottom
            int frame = 111 - keyIndices[vunion] - CountBits(vunion ^ masks[2]);
            if(111 != frame && 109 != frame)
            {
                c->backgroundLayer.BG0_drawPartialAsset(Vec2(5,10), Vec2(2,0), Vec2(4,1), MajorS, frame);
                c->foregroundLayer.DrawAsset(Vec2(5,10), Transparent_4x1);
            }

            if(109 == frame)
                c->foregroundLayer.DrawAsset(Vec2(3,10), MajorS_Frame109_AccentDigit);

        }


    }
    else
    {
        c->foregroundLayer.DrawPartialAsset(Vec2(3,1), Vec2(0,0), Vec2(MajorN.width, 2), MajorN, keyIndices[vunion] + CountBits(vunion ^ masks[0]));
        c->foregroundLayer.DrawPartialAsset(Vec2(3,11),Vec2(0,1), Vec2(MajorS.width, MajorS.height-1), MajorS, 111 - keyIndices[vunion] - CountBits(vunion ^ masks[2]));
    }
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
    static const PinnedAssetImage * const assets[20] =
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

    int digit = renderedDigit;//token->val;
    if(digit >=0 && digit <= 9)
    {
        int index = useAccentDigit? digit + 10 : digit;
        Vec2 p;
        p.x = 8*(TokenView::Mid.x - assets[index]->width / 2);
        p.y = 8*(TokenView::Mid.y - assets[index]->height / 2);
        GetCube()->backgroundLayer.setSpriteImage(0, *assets[index], 0);
        GetCube()->backgroundLayer.moveSprite(0, p);
    }

}


Fraction TokenGroup::OpHelper::Compute(Fraction left, Op op, Fraction right)
{
    Fraction ret;
    switch(op)
    {
    case OpAdd:
        ret = left + right;
        break;
    case OpSubtract:
        ret = left - right;
        break;
    case OpMultiply:
        ret = left * right;
        break;
    case OpDivide:
        ret = left / right;
        break;
    }
    return ret;
}


// // // //
}










