#include "TiltFlowView.h"
#include "TiltFlowDetailView.h"
#include "assets.gen.h"
#include "TiltFlowItem.h"
#include "Skins.h"

namespace TotalsGame {

const Sifteo::AssetImage *TiltFlowView::kMarquee[2] = {&Tilt_For_More, &Press_To_Select };

const float TiltFlowView::kMinAccel = 1;
const float TiltFlowView::kMaxAccel = 2.5f;


TiltFlowView::TiltFlowView():
    eventHandler(this)
{
    mItem = 0;
    mOffsetX = 0;
    mAccel = kMinAccel;
    mRestTime = -1;
    mDrawLabel = true;
    mDirty = true;
    mMarquee = -1;
    mLastUpdate = 0;
    mLastTileX = -1;
}

int TiltFlowView::GetItemIndex()
{
    return mItem;
}

TiltFlowItem *TiltFlowView::GetItem()
{
    return menu->GetItem(mItem);
}

void TiltFlowView::Tick()
{
    const float kUpdateTime = 1.0f / 50.0f;
    while(menu->GetSimTime() - mLastUpdate > kUpdateTime) {
        if (menu->IsPicked()) {
            menu->GetDetails()->message = "";
            menu->GetDetails()->HideDescription();            
            CoastToStop();
        } else {
            UpdateMenu();
        }
        if (mRestTime > -1 && menu->GetSimTime() - mRestTime > TiltFlowMenu::kRestDelay) {
            mDirty = true;
            mDrawLabel = true;
            mRestTime = -1;
        }
        mLastUpdate += kUpdateTime;
    }

    TotalsCube *c = GetCube();

    int newMarquee = menu->GetSimTime() / kMarqueeDelay;
    if(menu->IsPicked())
        newMarquee = -1;
    if (newMarquee != mMarquee)
    {
        mMarquee = newMarquee;
        c->foregroundLayer.DrawPartialAsset(vec(0,0), vec(0, 15), vec(16,1), Skins::GetSkin().vault_door); // header image
        PaintFooter(c);
        c->foregroundLayer.Flush();
    }

    if (mDirty)
    {
        mLastTileX = -1;
        PaintInner(c);
    }
}

//-------------------------------------------------------------------------
// VIEW METHODS
//-------------------------------------------------------------------------

void TiltFlowView::DidAttachToCube(TotalsCube *c) {
    c->AddEventHandler(&eventHandler);
    c->FillArea(&Dark_Purple, vec(0, 0), vec(18, 18));
}

void TiltFlowView::WillDetachFromCube(TotalsCube *c) {
    c->RemoveEventHandler(&eventHandler);
    c->foregroundLayer.Flush();
}

void TiltFlowView::PaintFooter(TotalsCube *c) {
    if(menu->IsPicked())
    {
        c->foregroundLayer.DrawPartialAsset(vec(0, 12), vec(0,0), vec(16,4), Skins::GetSkin().vault_door);
    }
    else
    {
        c->foregroundLayer.DrawAsset(vec(0, 12), *kMarquee[mMarquee % 2]);
    }
}

void TiltFlowView::PaintInner(TotalsCube *c) {    
    if(menu->IsDone())
    {
        GetCube()->FillArea(&Dark_Purple, vec(0,0), vec(18,18));
        GetCube()->Image(*menu->GetItem(mItem)->GetImage(), vec(3, 1), 0);
        GetCube()->backgroundLayer.BG0_setPanning(vec(0,0));
        return;
    }
    //left edge of center item
    int screenX = 96 * mItem - mOffsetX;
    int tileX = screenX / 8;
    int scrollX = screenX % 8;

    if(tileX != mLastTileX)
    {
        mLastTileX = tileX;
        int centerLeftTile = 12 * mItem;
        int baseScreenTileX = 3 + centerLeftTile - tileX;

        GetCube()->FillArea(&Dark_Purple, vec(0,3), vec(18,11));

        if(mItem > 0 && !menu->IsPicked())
            DoPaintItem(menu->GetItem(mItem-1), baseScreenTileX - 12);
        DoPaintItem(GetItem(), baseScreenTileX);
        if(mItem < menu->GetNumItems()-1 && !menu->IsPicked())
            DoPaintItem(menu->GetItem(mItem+1), baseScreenTileX + 12);
    }

    GetCube()->backgroundLayer.BG0_setPanning(vec(scrollX, 16));
}

void TiltFlowView::DoPaintItem(TiltFlowItem *item, int x) {
    if (item->HasImage()) {
        GetCube()->ClipImage(item->GetImage(), vec(x, 3));
    }
}

//-------------------------------------------------------------------------
// EVENTS
//-------------------------------------------------------------------------

void TiltFlowView::OnButton(TotalsCube *c, bool pressed) {
    if (!pressed && !menu->IsPicked()) {
        PLAY_SFX(sfx_Menu_Tilt_Stop);
        if (GetItem()->IsToggle()) {
            GetItem()->IncrementImageIndex();
            menu->SetToggledItem(GetItem());
            mDirty = true;
        }
        else if (GetItem()->id != TiltFlowItem::Passive)  // hack s.t. "locked" items won't be selectable
        {
            StopScrolling();
            menu->Pick();
            mDirty = true;
        }

    }
}

//-------------------------------------------------------------------------
// HELPERS
//-------------------------------------------------------------------------

void TiltFlowView::UpdateMenu() {

    bool showingDetails = GetCube()->DoesNeighbor(menu->GetDetails()->GetCube());
    if (!showingDetails) {
        menu->GetDetails()->HideDescription();
    }

    if (
            GetCube()->GetTilt().x == 1 ||
            (GetCube()->GetTilt().x < 1 && mItem == menu->GetNumItems()-1 && mOffsetX >= 0) ||
            (GetCube()->GetTilt().x > 1 && mItem == 0 && mOffsetX <= 0)
            )
    {
        AudioPlayer::HaltSfx(sfx_Menu_Tilt);
        CoastToStop();

        if (showingDetails)
        {
            menu->GetDetails()->ShowDescription(mOffsetX == 0 ? GetItem()->description : " ");
        }

    }
    else
    {
        if (showingDetails)
        {
            menu->GetDetails()->ShowDescription(" ");
        }

        mRestTime = menu->GetSimTime();

        PLAY_SFX2(sfx_Menu_Tilt, false);

        // accelerate in the direction of tilt
        mDrawLabel = false;
        int vSign = GetCube()->GetTilt().x < 1 ? -1 : 1;

        // if we're way overtilted (into the upper corners!), accelerate super-fast
        bool deepMult = GetCube()->GetTilt().x == 2 ? kDeepTiltAccel : 1;

        if (vSign > 0) {
            if (mItem > 0) {
                if (mOffsetX > 48) { // magic
                    mItem--;
                    mOffsetX -= 96; // magic
                } else {
                    mOffsetX += kTiltVel * mAccel;
                    mAccel = clamp(mAccel * kGravity, kMinAccel, kMaxAccel) * deepMult;
                }
            }
        } else {
            if (mItem < menu->GetNumItems()-1) {
                if (mOffsetX < -48) { // magic
                    mItem++;
                    mOffsetX += 96; // magic
                } else {
                    mOffsetX -= kTiltVel * mAccel;
                    mAccel = clamp(mAccel * kGravity, kMinAccel, kMaxAccel) * deepMult;
                }
            }
        }
        mDirty = true;
    }
}

void TiltFlowView::CoastToStop() {
    const float kEpsilon = 0.001f;
    if (abs(mOffsetX)  > kEpsilon) {
        mAccel = clamp(mAccel/kGravity, kMinAccel, kMaxAccel);
        mRestTime = menu->GetSimTime();
        if (abs(mOffsetX) < 2) { // magic
            StopScrolling();
        } else if (mOffsetX > 0) {
            mOffsetX -= kDriftVel * mAccel;
            if (mOffsetX < 0) { mAccel = kMinAccel; }
        } else {
            mOffsetX += kDriftVel * mAccel;
            if (mOffsetX > 0) { mAccel = kMinAccel; }
        }
        mDirty = true;
    }
}

void TiltFlowView::StopScrolling() {
    mOffsetX = 0;
    mAccel = kMinAccel;
    mRestTime = menu->GetSimTime();
    mDrawLabel = false;
    mDirty = true;
    mLastTileX = -1;
}

}

