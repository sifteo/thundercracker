#include "TotalsCube.h"
#include "assets.gen.h"
#include "Game.h"
#include "AudioPlayer.h"
#include "TokenView.h"
#include "DialogWindow.h"
#include "Skins.h"

namespace TotalsGame
{
    TotalsCube::TotalsCube():
        Sifteo::CubeID()
	{
		view = NULL;
		eventHandler = NULL;       
        overlayShown = false;                     
	}

    void TotalsCube::Init(Sifteo::CubeID id)
    {
        sys = id;
        vid.attach(*this);
    }

    void TotalsCube::AddEventHandler(EventHandler *e)
    {
        if(e->attached)
        {
            return;
        }
        e->attached = true;

        e->next = eventHandler;
        e->prev = NULL;
        if(e->next)
        {
            e->next->prev = e;
        }
        eventHandler = e;
    }

    void TotalsCube::RemoveEventHandler(EventHandler *e)
    {
        if(!e->attached)
        {
            return;
        }
        e->attached = false;

        //make sure this guy is even part of our list before we go and do things
        EventHandler *c = eventHandler;
        while(c)
        {
            if(c == e)
            {   //found it!
                if(e->prev)
                {
                    e->prev->next = e->next;
                }
                else
                {
                    ASSERT(e == eventHandler);
                    eventHandler = e->next;
                }

                if(e->next)
                {
                    e->next->prev = e->prev;
                }
                e->next = e->prev = NULL;
                return;
            }

            c = c->next;
        }


    }

    void TotalsCube::SetView(View *v)
    {
        if(v != view)            
        {
            if(view)
            {
                view->WillDetachFromCube(this);
            }
            view = v;
            if(view)
            {
                view->mCube = this;
                if(view)
                {
                    view->DidAttachToCube(this);
                }
            }
        }

    }

    bool TotalsCube::DoesNeighbor(TotalsCube *other)
    {
        Neighborhood n(*this);
        for(int i = 0; i < NUM_SIDES; i++)
        {
            if(n.neighborAt((Sifteo::Side)i).sys == other->sys)
                return true;
        }
        return false;
    }


    void TotalsCube::ResetEventHandlers()
    {
        while(eventHandler)
        {
            RemoveEventHandler(eventHandler);
        }
    }

    void TotalsCube::DispatchOnCubeShake(TotalsCube *c)
    {
        EventHandler *e = eventHandler;
        while(e)
        {
            e->OnCubeShake(c);
            e = e->next;
        }
    }

    void TotalsCube::DispatchOnCubeTouch(TotalsCube *c, bool touching)
    {
        EventHandler *e = eventHandler;
        while(e)
        {
            e->OnCubeTouch(c, touching);
            e = e->next;
        }
    }
	
    void TotalsCube::Image(const Sifteo::AssetImage &image, Int2 pos, int frame)
	{
        vid.bg0.image(pos, image, frame);
	}

    void TotalsCube::Image(const Sifteo::AssetImage *image, const Int2 &coord, const Int2 &offset, const Int2 &size)
	{
        vid.bg0.image(coord, size, *image, offset);
	}

    void TotalsCube::FillArea(const Sifteo::PinnedAssetImage *image, Int2 pos, Int2 size)
    {
        Int2 p = pos;
        Int2 s = size;

        if(p.x < 0)
        {
            s.x += p.x;
            p.x = 0;
        }

        if(p.y < 0)
        {
            s.y += p.y;
            p.y = 0;
        }

        if(p.x + s.x > 18)
        {
            s.x = 18 - p.x;
        }

        if(p.y + s.y > 18)
        {
            s.y = 18 - p.y;
        }

        if(s.x <= 0 || s.y <= 0)
        {
            return;
        }

        vid.bg0.fill(p, s, *image);
    }

    void TotalsCube::Image(const Sifteo::AssetImage &image)
    {
        vid.bg0.image(vec(0,0), image);
    }

    void TotalsCube::ClipImage(const Sifteo::AssetImage *image, Int2 pos)
    {
        Int2 p = pos;
        Int2 o = vec(0,0);
        Int2 s = vec(image->tileWidth(), image->tileHeight());

        if(p.x < 0)
        {
            o.x -= p.x;
            s.x += p.x;
            p.x = 0;
        }

        if(p.y < 0)
        {
            o.y -= p.y;
            s.y += p.y;
            p.y = 0;
        }

        if(p.x + s.x > 18)
        {
            s.x = 18 - p.x;
        }

        if(p.y + s.y > 18)
        {
            s.y = 18 - p.y;
        }

        if(s.x <= 0 || s.y <= 0)
        {
            return;
        }

        Image(image, p, o, s);

    }

	// for these methods, 0 <= offset <= 32

	void TotalsCube::DrawVaultDoorsClosed()
	{
		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

        const Skins::Skin &skin = Skins::GetSkin();

        vid.bg0.image(vec(0,0),vec(x,y), skin.vault_door,vec(9,9));
        vid.bg0.image(vec(x,0),vec(16-x,y), skin.vault_door,vec(0,9));
        vid.bg0.image(vec(0,y),vec(x,16-y), skin.vault_door,vec(9,0));
        vid.bg0.image(vec(x,y),vec(16-x,16-y), skin.vault_door,vec(0,0));
	}

    void TotalsCube::OpenShuttersToReveal(const Sifteo::AssetImage &image)
    {	
		AudioPlayer::PlayShutterOpen();
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            Image(image);
            DrawVaultDoorsOpenStep1(32.0f * t/kTransitionTime);
            System::paint();
            //System::finish();
            Game::UpdateDt();
		}
        
        Image(image);
        DrawVaultDoorsOpenStep1(32);
        System::paint();
        //System::finish();
        Game::UpdateDt();

        
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            Image(image);
            DrawVaultDoorsOpenStep2(32.0f * t/kTransitionTime);
            System::paint();
            //System::finish();
            Game::UpdateDt();
		}
        
        Image(image, vec(0,0));
    }

    void TotalsCube::CloseShutters()
    {
        AudioPlayer::PlayShutterClose();
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            DrawVaultDoorsOpenStep2(32.0f - 32.0f * t/kTransitionTime);
            System::paint();
            //System::finish();
            Game::UpdateDt();
		}
        
        DrawVaultDoorsOpenStep2(0);
        System::paint();
        //System::finish();
        Game::UpdateDt();
        
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            DrawVaultDoorsOpenStep1(32.0f - 32.0f * t/kTransitionTime);
            System::paint();
            //System::finish();
            Game::UpdateDt();
		}			

        DrawVaultDoorsClosed();
        System::paint();

    }



    void TotalsCube::DrawVaultDoorsOpenStep1(int offset)
	{
		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int yTop = x - (offset+4)/8;
		int yBottom = y + (offset+4)/8;

        const Skins::Skin &skin = Skins::GetSkin();

        vid.bg0.image(vec(0,0), vec(x,yTop),skin.vault_door, vec(16-x,16-yTop));			//Top left
        vid.bg0.image(vec(x,0), vec(16-x,yTop),skin.vault_door, vec(0,16-yTop));			//top right
        vid.bg0.image(vec(0,yBottom), vec(x,16-yBottom),skin.vault_door, vec(16-x,0));	//bottom left
        vid.bg0.image(vec(x,yBottom), vec(16-x,16-yBottom),skin.vault_door, vec(0,0));	//bottom right
		/*
		if (innerImage && yTop != yBottom) 
		{
			mode.BG0_drawPartialAsset(vec(0,yTop), vec(0,yTop), vec(16,yBottom-yTop), *innerImage); // "inner" row
		} 
		*/
	}

    void TotalsCube::DrawVaultDoorsOpenStep2(int offset)
	{
		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int xLeft = x - (offset+4)/8;
		int xRight = y + (offset+4)/8;

        const Skins::Skin &skin = Skins::GetSkin();

        vid.bg0.image(vec(0,0), vec(xLeft,y-4),skin.vault_door, vec(16-xLeft,16-(y-4)));			//Top left
        vid.bg0.image(vec(xRight,0), vec(16-xRight,y-4),skin.vault_door, vec(0,16-(y-4)));		//Top right
        vid.bg0.image(vec(0,y+4), vec(xLeft,16-(y+4)),skin.vault_door, vec(16-xLeft,0));			//bottom left
        vid.bg0.image(vec(xRight,y+4), vec(16-xRight,16-(y+4)),skin.vault_door, vec(0,0));			//bottom right
		/*
		if (innerImage && xLeft != xRight) {
			mode.BG0_drawPartialAsset(vec(xLeft,0), vec(xLeft,0), vec(xRight-xLeft,16), *innerImage); // "inner" column
		}
		*/
	}


    void TotalsCube::DrawFraction(Fraction f, Int2 pos)
    {
        Sifteo::String<10> string;
        f.ToString(&string);
        DrawString(string, pos);
    }

    void TotalsCube::DrawString(const char *string, Int2 center)
    {
        int hw = 0;
        const char *s = string;
        //find string halfwidth
        while(*s)
        {
            switch(*s)
            {
            case '-':
                hw += 5;
                break;
            case '/':
                hw += 6;
                break;
            case '.':
                hw += 4;
                break;
            default:
                hw += 6;
                break;
            }
            s++;
        }
        Int2 p = center - vec(hw, 8);

        int curSprite = 0;
        s = string;
        while(*s)
        {
            switch(*s)
            {
            case '-':
                vid.sprites[curSprite].setImage(Digits, 10);
                vid.sprites[curSprite].move(p + vec(0,3));
                p.x += 10-1;
                break;
            case '/':
                vid.sprites[curSprite].setImage(Digits, 12);
                vid.sprites[curSprite].move(p);
                p.x += 12-1;
                break;
            case '.':
                vid.sprites[curSprite].setImage(Digits, 11);
                vid.sprites[curSprite].move(p + vec(0,12-7));
                p.x += 8-1;
                break;
            default:
                vid.sprites[curSprite].setImage(Digits, (*s)-'0');
                vid.sprites[curSprite].move(p);
                p.x += 12-1;
                break;
            }
            s++;
            curSprite++;
        }


    }

    void TotalsCube::EnableTextOverlay(const char *text, int yTop, int ySize, int fg[3], int bg[3])
    {
        DialogWindow dw(this);
        dw.SetBackgroundColor(bg[0], bg[1], bg[2]);
        dw.SetForegroundColor(fg[0], fg[1], fg[2]);
        dw.DoDialog(text, yTop, ySize);
        overlayShown = true;
    }

    void TotalsCube::ChangeOverlayText(const char *text)
    {
        if(overlayShown)
        {
            DialogWindow dw(this);
            dw.ChangeText(text);
        }
    }

    void TotalsCube::DisableTextOverlay()
    {
        //turn it off
        vid.initMode(BG0_SPR_BG1);
        view->Paint();
        System::paint();

        overlayShown = false;
    }

    bool TotalsCube::IsTextOverlayEnabled()
    {
        return overlayShown;
    }

    void TotalsCube::HideSprites()
    {
        vid.sprites.erase();
    }

}
