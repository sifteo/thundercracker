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
        const char *overlayText;
        int overlayYTop, overlayYSize;
        int overlayBg[3], overlayFg[3];
        bool overlayShown;

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

        void SetView(View *v);

		View *GetView(void) {return view;}

        bool DoesNeighbor(TotalsCube *other);

        Vec2 GetTilt();

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
        void Image(const PinnedAssetImage *image, const Vec2 &coord, int frame);
        void ClipImage(const AssetImage *image, const Vec2 &pos);
        void FillScreen(const AssetImage *image);
	
        void DrawFraction(Fraction f, const Vec2 &pos);
        //void DrawDecimal(float d, const Vec2 &pos);
        void DrawString(const char *string, const Vec2 &center);

        void EnableTextOverlay(const char *text, int yTop, int ySize, int br, int bg, int bb, int fr, int fg, int fb);
        void DisableTextOverlay();
        void UpdateTextOverlay();
        bool IsTextOverlayEnabled();

        void DispatchOnCubeShake(TotalsCube *c);
        void DispatchOnCubeTouch(TotalsCube *c, bool touching);

//	private:

		// for these methods, 0 <= offset <= 32
		void DrawVaultDoorsOpenStep1(int offset, const AssetImage *innerImage);
		void DrawVaultDoorsOpenStep2(int offset, const AssetImage *innerImage);
	};

}
