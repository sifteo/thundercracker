#include "TotalsCube.h"
#include "assets.gen.h"
#include "Game.h"
#include "AudioPlayer.h"
#include "TokenView.h"
#include "string.h"

namespace TotalsGame
{
    TotalsCube::TotalsCube():
        Sifteo::Cube(),
      backgroundLayer(this->vbuf),
      foregroundLayer(*this)
	{
		CORO_RESET;
		view = NULL;
		eventHandler = NULL;       
        //backgroundLayer.init();
        backgroundLayer.set();
	}

    void TotalsCube::AddEventHandler(EventHandler *e)
    {
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
        if(e->prev)
        {
            e->prev->next = e->next;
        }
        else
        {
            assert(e == eventHandler);
            eventHandler = e->next;
        }

        if(e->next)
        {
            e->next->prev = e->prev;
        }
        e->next = e->prev = NULL;
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


	float TotalsCube::OpenShutters(const AssetImage *image)
	{						
		CORO_BEGIN

		AudioPlayer::PlayShutterOpen();
		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
		{
			DrawVaultDoorsOpenStep1(32.0f * t/kTransitionTime, image);
			CORO_YIELD(0);
		}

		DrawVaultDoorsOpenStep1(32, image);			
		CORO_YIELD(0);

		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt)
		{
			DrawVaultDoorsOpenStep2(32.0f * t/kTransitionTime, image);
			CORO_YIELD(0);
		}
		CORO_END
		CORO_RESET;

		return -1;
	}

	float TotalsCube::CloseShutters(const AssetImage *image)
	{
		CORO_BEGIN

		AudioPlayer::PlayShutterClose();
		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
		{
			DrawVaultDoorsOpenStep2(32.0f - 32.0f * t/kTransitionTime, image);
			CORO_YIELD(0);
		}

		DrawVaultDoorsOpenStep2(0, image);
		CORO_YIELD(0);

		for(t=0.0f; t<kTransitionTime; t+=Game::GetInstance().dt) 
		{
			DrawVaultDoorsOpenStep1(32.0f - 32.0f * t/kTransitionTime, image);				
			CORO_YIELD(0);
		}			

		CORO_END
		CORO_RESET;

		return -1;
	}
	
    void TotalsCube::Image(const AssetImage *image, const Vec2 &pos)
	{
        backgroundLayer.BG0_drawAsset(pos, *image, 0);
	}

	void TotalsCube::Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size)
	{
        backgroundLayer.BG0_drawPartialAsset(coord, offset, size, *image, 0);
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

        if(p.x + s.x > 16)
        {
            s.x = 16 - p.x;
        }

        if(p.y + s.y > 16)
        {
            s.y = 16 - p.y;
        }

        if(s.x == 0 || s.y == 0)
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

        backgroundLayer.BG0_drawPartialAsset(Vec2(0,0),Vec2(9,9),Vec2(x,y), VaultDoor, 0);
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,0),Vec2(0,9),Vec2(16-x,y), VaultDoor, 0);
        backgroundLayer.BG0_drawPartialAsset(Vec2(0,y),Vec2(9,0),Vec2(x,16-y), VaultDoor, 0);
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,y),Vec2(0,0),Vec2(16-x,16-y), VaultDoor, 0);
	}

	void TotalsCube::DrawVaultDoorsOpenStep1(int offset, const AssetImage *innerImage) 
	{
		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int yTop = x - (offset+4)/8;
		int yBottom = y + (offset+4)/8;

		if(innerImage)
            backgroundLayer.BG0_drawAsset(Vec2(0,0), *innerImage);

        backgroundLayer.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-x,16-yTop), Vec2(x,yTop), VaultDoor);			//Top left
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,0), Vec2(0,16-yTop), Vec2(16-x,yTop), VaultDoor);			//top right
        backgroundLayer.BG0_drawPartialAsset(Vec2(0,yBottom), Vec2(16-x,0), Vec2(x,16-yBottom), VaultDoor);	//bottom left
        backgroundLayer.BG0_drawPartialAsset(Vec2(x,yBottom), Vec2(0,0), Vec2(16-x,16-yBottom), VaultDoor);	//bottom right

		/*
		if (innerImage && yTop != yBottom) 
		{
			mode.BG0_drawPartialAsset(Vec2(0,yTop), Vec2(0,yTop), Vec2(16,yBottom-yTop), *innerImage); // "inner" row
		} 
		*/
	}

	void TotalsCube::DrawVaultDoorsOpenStep2(int offset, const AssetImage *innerImage) 
	{
		const int x = TokenView::Mid.x;
		const int y = TokenView::Mid.y;

		int xLeft = x - (offset+4)/8;
		int xRight = y + (offset+4)/8;

		if(innerImage)
            backgroundLayer.BG0_drawAsset(Vec2(0,0), *innerImage);

        backgroundLayer.BG0_drawPartialAsset(Vec2(0,0), Vec2(16-xLeft,16-(y-4)), Vec2(xLeft,y-4), VaultDoor);			//Top left
        backgroundLayer.BG0_drawPartialAsset(Vec2(xRight,0), Vec2(0,16-(y-4)), Vec2(16-xRight,y-4), VaultDoor);		//Top right
        backgroundLayer.BG0_drawPartialAsset(Vec2(0,y+4), Vec2(16-xLeft,0), Vec2(xLeft,16-(y+4)), VaultDoor);			//bottom left
        backgroundLayer.BG0_drawPartialAsset(Vec2(xRight,y+4), Vec2(0,0), Vec2(16-xRight,16-(y+4)), VaultDoor);			//bottom right
		
		/*
		if (innerImage && xLeft != xRight) {
			mode.BG0_drawPartialAsset(Vec2(xLeft,0), Vec2(xLeft,0), Vec2(xRight-xLeft,16), *innerImage); // "inner" column
		}
		*/
	}


    void TotalsCube::DrawFraction(Fraction f, const Vec2 &pos)
    {
        char string[10];
        f.ToString(string, 10);
        DrawString(string, pos);
    }

    void TotalsCube::DrawDecimal(float d, const Vec2 &pos)
    {
        char string[10];
        snprintf(string, 10, "%f", d);

        char *dot = strchr(string, '.');
        if (!dot)
        {
                DrawString(string, pos);
        }
        else
        {
            int decimalCount = 1;
            while(decimalCount < 3 && *(dot+decimalCount))
                decimalCount++;
            *(dot+decimalCount) = 0;
            DrawString(string, pos);
        }

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


}
