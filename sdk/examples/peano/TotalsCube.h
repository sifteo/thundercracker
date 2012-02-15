#pragma once

#include "sifteo.h"
#include "coroutine.h"
#include "View.h"
#include "Fraction.h"

namespace TotalsGame
{

	

	class TotalsCube: public Sifteo::Cube
	{
		CORO_PARAMS;
		float t;

		View *view;

	public:
        //TODO duplicate work in processing bg1!
        VidMode_BG0_SPR_BG1 backgroundLayer;
        BG1Helper foregroundLayer;

		class EventHandler
		{            
            //event handlers will be chained a linked list
            EventHandler *prev, *next;
            friend class TotalsCube;
		public:
			virtual void OnCubeShake(TotalsCube *cube) {}
			virtual void OnCubeTouch(TotalsCube *cube, bool touching) {}
		};

		TotalsCube();
		static const float kTransitionTime = 0.1f;

        void SetView(View *v)
        {
            if(v != view)
            {
                delete view;
                view = v;
            }

        }

		View *GetView(void) {return view;}

private:
		EventHandler *eventHandler;

public:
        void AddEventHandler(EventHandler *e);
        void RemoveEventHandler(EventHandler *e);
        void ResetEventHandlers();

		float OpenShutters(const AssetImage *image);
		float CloseShutters(const AssetImage *image);
				
		void DrawVaultDoorsClosed();

        void Image(const AssetImage *image, const Vec2 &pos);
		void Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size);
	
        void DrawFraction(Fraction f, const Vec2 &pos);
        void DrawDecimal(float d, const Vec2 &pos);
        void DrawString(const char *string, const Vec2 &center);

        void DispatchOnCubeShake(TotalsCube *c);
        void DispatchOnCubeTouch(TotalsCube *c, bool touching);

	private:

		// for these methods, 0 <= offset <= 32
		void DrawVaultDoorsOpenStep1(int offset, const AssetImage *innerImage);
		void DrawVaultDoorsOpenStep2(int offset, const AssetImage *innerImage);
	};

}
