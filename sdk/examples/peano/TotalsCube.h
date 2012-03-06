#pragma once

#include "sifteo.h"
#include "View.h"
#include "Fraction.h"

namespace TotalsGame
{

	

	class TotalsCube: public Sifteo::Cube
	{
		View *view;
        const char *overlayText;
        int overlayYTop, overlayYSize;
        int overlayBg[3], overlayFg[3];
        bool overlayShown;

    public:
        VidMode_BG0_SPR_BG1 backgroundLayer;
        BG1Helper foregroundLayer;

		class EventHandler
		{            
            //event handlers will be chained a linked list
            EventHandler *prev, *next;
            bool attached;
            friend class TotalsCube;
		public:
            EventHandler():prev(0),next(0),attached(false) {}
			virtual void OnCubeShake(TotalsCube *cube) {}
			virtual void OnCubeTouch(TotalsCube *cube, bool touching) {}
		};

		TotalsCube();
		static const float kTransitionTime = 0.1f;

        void SetView(View *v);

		View *GetView(void) {return view;}

        bool DoesNeighbor(TotalsCube *other);

        Vec2 GetTilt();

        void HideSprites();

private:
		EventHandler *eventHandler;

public:
        void AddEventHandler(EventHandler *e);
        void RemoveEventHandler(EventHandler *e);
        void ResetEventHandlers();

        float OpenShuttersAsync(const AssetImage *image);
        float CloseShuttersAsync(const AssetImage *image);
				
        void OpenShuttersSync(const AssetImage *image);
        void CloseShuttersSync(const AssetImage *image);

		void DrawVaultDoorsClosed();

        void Image(const AssetImage *image, const Vec2 &pos, int frame=0);
		void Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size);
        void Image(const PinnedAssetImage *image, const Vec2 &coord, int frame=0);
        void ClipImage(const AssetImage *image, const Vec2 &pos);
        void ClipImage(const PinnedAssetImage *image, const Vec2 &pos, int frame = 0);
        void FillArea(const AssetImage *image, const Vec2 &pos, const Vec2 &size);
	
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
