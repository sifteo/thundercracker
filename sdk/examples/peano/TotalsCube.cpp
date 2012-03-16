#include "TotalsCube.h"
#include "assets.gen.h"
#include "Game.h"
#include "AudioPlayer.h"
#include "TokenView.h"
#include "string.h"
#include "DialogWindow.h"
#include "Skins.h"

namespace TotalsGame
{
    TotalsCube::TotalsCube():
        Sifteo::Cube(),
      backgroundLayer(this->vbuf),
        foregroundLayer(*this)
	{
		view = NULL;
		eventHandler = NULL;       
        //backgroundLayer.init();
        backgroundLayer.set();
        foregroundLayer.Flush();

        overlayShown = false;        
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

    Vec2 TotalsCube::GetTilt()
    {
        Cube::TiltState s = getTiltState();return Vec2(s.x, s.y);
    }

    bool TotalsCube::DoesNeighbor(TotalsCube *other)
    {
        for(int i = 0; i < NUM_SIDES; i++)
        {
            if(physicalNeighborAt(i) == other->id())
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
	
    void TotalsCube::Image(const AssetImage &image, const Vec2 &pos, int frame)
	{
        backgroundLayer.BG0_drawAsset(pos, image, frame);
	}

	void TotalsCube::Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size)
	{
        backgroundLayer.BG0_drawPartialAsset(coord, offset, size, *image, 0);
	}

    void TotalsCube::Image(const PinnedAssetImage *image, const Vec2 &coord, int frame)
    {
        int tile = image->index + image->width * image->height * frame;
        for(int y = coord.y; y < coord.y + (int)image->height; y++)
        {
            for(int x = coord.x; x < coord.x + (int)image->width; x++)
            {
                backgroundLayer.BG0_putTile(Vec2(x,y), tile++);
            }
        }
    }

    void TotalsCube::ClipImage(const PinnedAssetImage *image, const Vec2 &pos, int frame)
    {
        int tile = image->index + image->width * image->height * frame;
        int y = pos.y;
        int maxy = y + image->height;
        if(y < 0)
        {
            tile += -y*image->width;
            y = 0;
        }
        if(maxy > 18)
        {
            maxy = 18;
        }
        for(; y < maxy; y++)
        {

            int x = pos.x;
            int maxx = x + image->width;
            int tileSkip = tile + image->width;
            if(x < 0)
            {
                tile += -x;
                x = 0;
            }
            if(maxx > 18)
            {
                maxx = 18;
            }

            for(;x < maxx; x++)
            {
                backgroundLayer.BG0_putTile(Vec2(x,y), tile++);
            }

            tile = tileSkip;

        }
    }

    void TotalsCube::FillArea(const AssetImage *image, const Vec2 &pos, const Vec2 &size)
    {
        Vec2 p = pos;
        Vec2 s = size;

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


        for(int y = p.y; y < p.y + s.y; y++)
        {
            for(int x = p.x; x < p.x + s.x; x++)
            {
                backgroundLayer.BG0_putTile(Vec2(x,y), image->tiles[0]);
            }
        }
    }

    void TotalsCube::Image(const AssetImage &image)
    {
        backgroundLayer.BG0_drawAsset(Vec2(0,0), image);
    }

    void TotalsCube::ClipImage(const AssetImage *image, const Vec2 &pos)
    {
        Vec2 p = pos;
        Vec2 o(0,0);
        Vec2 s(image->width, image->height);

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

        backgroundLayer.BG0_drawPartialAsset(Vec2(0,0),Vec2(9,9),Vec2(x,y), skin.vault_door, 0);
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,0),Vec2(0,9),Vec2(16-x,y), skin.vault_door, 0);
        backgroundLayer.BG0_drawPartialAsset(Vec2(0,y),Vec2(9,0),Vec2(x,16-y), skin.vault_door, 0);
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,y),Vec2(0,0),Vec2(16-x,16-y), skin.vault_door, 0);
	}

    void TotalsCube::OpenShuttersToReveal(const AssetImage &image)
    {	
		AudioPlayer::PlayShutterOpen();
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            Image(image);
            DrawVaultDoorsOpenStep1(32.0f * t/kTransitionTime);
            System::paintSync();
            Game::UpdateDt();
		}
        
        Image(image);
        DrawVaultDoorsOpenStep1(32);
        System::paintSync();
        Game::UpdateDt();

        
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            Image(image);
            DrawVaultDoorsOpenStep2(32.0f * t/kTransitionTime);
            System::paintSync();
            Game::UpdateDt();
		}
        
        Image(image, Vec2(0,0));
    }

    void TotalsCube::CloseShutters()
    {
        AudioPlayer::PlayShutterClose();
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            DrawVaultDoorsOpenStep2(32.0f - 32.0f * t/kTransitionTime);
			System::paintSync();
            Game::UpdateDt();
		}
        
        DrawVaultDoorsOpenStep2(0);
		System::paintSync();
        Game::UpdateDt();
        
        for(float t=0.0f; t<kTransitionTime; t+=Game::dt)
		{
            DrawVaultDoorsOpenStep1(32.0f - 32.0f * t/kTransitionTime);
			System::paintSync();
            Game::UpdateDt();
		}			

    }



    void TotalsCube::DrawVaultDoorsOpenStep1(int offset)
	{
		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int yTop = x - (offset+4)/8;
		int yBottom = y + (offset+4)/8;

        const Skins::Skin &skin = Skins::GetSkin();

        backgroundLayer.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-x,16-yTop), Vec2(x,yTop), skin.vault_door);			//Top left
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,0), Vec2(0,16-yTop), Vec2(16-x,yTop), skin.vault_door);			//top right
        backgroundLayer.BG0_drawPartialAsset(Vec2(0,yBottom), Vec2(16-x,0), Vec2(x,16-yBottom), skin.vault_door);	//bottom left
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,yBottom), Vec2(0,0), Vec2(16-x,16-yBottom), skin.vault_door);	//bottom right

		/*
		if (innerImage && yTop != yBottom) 
		{
			mode.BG0_drawPartialAsset(Vec2(0,yTop), Vec2(0,yTop), Vec2(16,yBottom-yTop), *innerImage); // "inner" row
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

        backgroundLayer.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-xLeft,16-(y-4)), Vec2(xLeft,y-4), skin.vault_door);			//Top left
        backgroundLayer.BG0_drawPartialAsset(Vec2(xRight,0), Vec2(0,16-(y-4)), Vec2(16-xRight,y-4), skin.vault_door);		//Top right
        backgroundLayer.BG0_drawPartialAsset(Vec2(0,y+4), Vec2(16-xLeft,0), Vec2(xLeft,16-(y+4)), skin.vault_door);			//bottom left
        backgroundLayer.BG0_drawPartialAsset(Vec2(xRight,y+4), Vec2(0,0), Vec2(16-xRight,16-(y+4)), skin.vault_door);			//bottom right
		
		/*
		if (innerImage && xLeft != xRight) {
			mode.BG0_drawPartialAsset(Vec2(xLeft,0), Vec2(xLeft,0), Vec2(xRight-xLeft,16), *innerImage); // "inner" column
		}
		*/
	}


    void TotalsCube::DrawFraction(Fraction f, const Vec2 &pos)
    {
        String<10> string;
        f.ToString(&string);
        DrawString(string, pos);
    }

    void TotalsCube::DrawString(const char *string, const Vec2 &center)
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
        Vec2 p = center - Vec2(hw, 8);

        int curSprite = 0;
        s = string;
        while(*s)
        {
            switch(*s)
            {
            case '-':
                backgroundLayer.setSpriteImage(curSprite, Digits, 10);
                backgroundLayer.moveSprite(curSprite, p + Vec2(0,3));
                p.x += 10-1;
                break;
            case '/':
                backgroundLayer.setSpriteImage(curSprite, Digits, 12);
                backgroundLayer.moveSprite(curSprite, p);
                p.x += 12-1;
                break;
            case '.':
                backgroundLayer.setSpriteImage(curSprite, Digits, 11);
                backgroundLayer.moveSprite(curSprite, p + Vec2(0,12-7));
                p.x += 8-1;
                break;
            default:
                backgroundLayer.setSpriteImage(curSprite, Digits, (*s)-'0');
                backgroundLayer.moveSprite(curSprite, p);
                p.x += 12-1;
                break;
            }
            s++;
            curSprite++;
        }


    }

    void TotalsCube::EnableTextOverlay(const char *text, int yTop, int ySize, int br, int bg, int bb, int fr, int fg, int fb)
    {
        DialogWindow dw(this);
        dw.SetBackgroundColor(br, bg, bb);
        dw.SetForegroundColor(fr, fg, fb);
        dw.DoDialog(text, yTop, ySize);
        overlayShown = true;
    }

    void TotalsCube::DisableTextOverlay()
    {
        //turn it off
        System::paintSync();

        backgroundLayer.set();
        backgroundLayer.clear();
        backgroundLayer.setWindow(0, 128);
        foregroundLayer.Clear();
        view->Paint();
        foregroundLayer.Flush();
        System::paintSync();

        overlayShown = false;
    }

    bool TotalsCube::IsTextOverlayEnabled()
    {
        return overlayShown;
    }

    void TotalsCube::HideSprites()
    {
        for(int i = 0; i < _SYS_VRAM_SPRITES; i++)
        {
            backgroundLayer.hideSprite(i);
        }
    }

}
