#include "TiltFlowView.h"
#include "TiltFlowDetailView.h"
#include "assets.gen.h"
#include "TiltFlowItem.h"

namespace TotalsGame {

const PinnedAssetImage *TiltFlowView::kMarquee[2] = {&Tilt_For_More, &Press_To_Select };

const float TiltFlowView::kMinAccel = 1;
const float TiltFlowView::kMaxAccel = 2.5f;

TiltFlowMenu *TiltFlowView::GetTiltFlowMenu()
{
    return menu;
}

TiltFlowView::TiltFlowView(TotalsCube *c, TiltFlowMenu *_menu):
    View(c), eventHandler(this)
{
    menu = _menu;

    mItem = 0;
    mOffsetX = 0;
    mAccel = kMinAccel;
    mRestTime = -1;
    mDrawLabel = true;
    mDirty = true;
    mMarquee = 0;
    mLastUpdate = 0;

    c->FillArea(&Dark_Purple, Vec2(0, 1), Vec2(16, 16-4-1));

    /* warning: big hack TODO
          view constructor calls virtual DidAttachToCube
          since we're currently in a constructor the virtual call
          goes to the base class, not *this* class.
          I'll just call it manually to make it work.
          */
    DidAttachToCube(c);
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

    bool wantsCubePaint = false;

    int newMarquee = menu->GetSimTime() / kMarqueeDelay;
    if (newMarquee != mMarquee) {
        mMarquee = newMarquee;
        if (OkayToPaint()) {
            PaintFooter(GetCube());
            wantsCubePaint = true;
        }
    }

    if (mDirty) {
        if (OkayToPaint()) {
            PaintInner(GetCube());
            wantsCubePaint = true;
        }
        mDirty = false;
    }

    if (wantsCubePaint) { GetCube()->GetView()->Paint(); }
}

//-------------------------------------------------------------------------
// VIEW METHODS
//-------------------------------------------------------------------------

void TiltFlowView::DidAttachToCube(TotalsCube *c) {
    c->AddEventHandler(&eventHandler);
}

void TiltFlowView::WillDetachFromCube(TotalsCube *c) {
    c->RemoveEventHandler(&eventHandler);
}

void TiltFlowView::Paint() {
    TotalsCube *c = GetCube();
    PaintInner(c);
    c->ClipImage(&VaultDoor, Vec2(0, 1-16)); // header image
    PaintFooter(c);
}

void TiltFlowView::PaintFooter(TotalsCube *c) {
    c->ClipImage(&VaultDoor, Vec2(0, 16-4));
    const PinnedAssetImage *image = kMarquee[mMarquee % 2];

    if(menu->IsPicked())
    {
        c->backgroundLayer.hideSprite(0)   ;
    }
    else
    {
        c->backgroundLayer.setSpriteImage(0, *image, 0);
        c->backgroundLayer.moveSprite(0, Vec2((128-8*image->width)>>1, 128-26));
    }
}

void TiltFlowView::PaintInner(TotalsCube *c) {    
    if (menu->IsPicked()) {
        int x, w;
        ClipIt(24, &x, &w); // magic
        if (w > 0) {
            DoPaintItem(GetItem(), x, w);
            c->FillArea(&Dark_Purple, Vec2(0, 1), Vec2(x/8, 16-4-1));
            c->FillArea(&Dark_Purple, Vec2((x+w)/8, 1), Vec2(16-(x+w)/8, 16-4-1));
            if (mDrawLabel) {
                //TODO Library.Verdana.Paint(c, Item.name, Int2.Zero, HorizontalAlignment.Center, VerticalAlignment.Middle, 1, 0, true, false, new Int2(128,20)); // magic
            }
        }
    } else {
        c->FillArea(&Dark_Purple, Vec2((mOffsetX+24+80)/8, 1), Vec2(2, 16-4-1));
        c->FillArea(&Dark_Purple, Vec2((mOffsetX-72+80)/8, 1), Vec2(2, 16-4-1));

        int x, w;
        ClipIt(24, &x, &w); // magic
        if (w > 0)
        {
            DoPaintItem(GetItem(), x, w);
        }
        if (mItem > 0)
        {
            ClipIt(-72, &x, &w); // magic
            if (w > 0)
            {
                DoPaintItem(menu->GetItem(mItem-1), x, w);
            }
        }
        if (mItem < menu->GetNumItems()-1)
        {
            ClipIt(120, &x, &w); // magic
            if (w > 0)
            {
                DoPaintItem(menu->GetItem(mItem+1),x, 80);
            }
        }
    }
}

void TiltFlowView::PixelToTileImage(const AssetImage *image, const Vec2 &p, const Vec2 &o, const Vec2 &s)
{
    //todo tiles or sprites?
    GetCube()->ClipImage(image, p/8);//, o/8, s/8);
}

void TiltFlowView::DoPaintItem(TiltFlowItem *item, int x, int w) {
    const int y = 12; // magic
    const int h = 80; // magic
    if (item->HasImage()) {
        if (x==0 && w<80) {
            PixelToTileImage(item->GetImage(), Vec2(w-80, y), item->GetSourcePosition(), Vec2(80, h)); //todo wtf is this extra parameters?! , 1, 0);
        } else {
            PixelToTileImage(item->GetImage(), Vec2(x, y), item->GetSourcePosition(), Vec2(w, h));//TODO same as above!, 1, 0);
        }
    }
    else
    {
        //TODO   Cube.FillRect(item.color, x, y, w, h);
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
            ) {
        //menu.firstTime = false;
        //TODO        Jukebox.SfxHalt("PV_menu_tilt");
        CoastToStop();

        if (showingDetails) {
            menu->GetDetails()->ShowDescription(mOffsetX == 0 ? GetItem()->description : " ");
        }

    } else /*if (!menu.firstTime)*/ {

        if (showingDetails) {
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
                if (mOffsetX > 45) { // magic
                    mItem--;
                    mOffsetX -= 90; // magic
                } else {
                    mOffsetX += kTiltVel * mAccel;
                    mAccel = clamp(mAccel * kGravity, kMinAccel, kMaxAccel) * deepMult;
                }
            }
        } else {
            if (mItem < menu->GetNumItems()-1) {
                if (mOffsetX < -45) { // magic
                    mItem++;
                    mOffsetX += 90; // magic
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
}

void TiltFlowView::ClipIt(int ox, int *x, int *w) {
    *x = ox + mOffsetX;  //TODO go back through this file and put the 'floor's back in
    *w = 80;
    if (*x < 0) {
        *w += *x;
        *x = 0;
    }
    *w -= (*x+*w>128) ? 80-(128-*x) : 0;
}

}

