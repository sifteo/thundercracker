#pragma once

#include "sifteo.h"
#include "View.h"
#include "Fraction.h"

using namespace Sifteo::Math;

namespace TotalsGame
{

	

	class TotalsCube: public Sifteo::Cube
	{
		View *view;
        bool overlayShown;

    public:
        Sifteo::VidMode_BG0_SPR_BG1 backgroundLayer;
        Sifteo::BG1Helper foregroundLayer;

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

        Vector2<int> GetTilt();

        void HideSprites();

private:
		EventHandler *eventHandler;

public:
        void AddEventHandler(EventHandler *e);
        void RemoveEventHandler(EventHandler *e);
        void ResetEventHandlers();

        void OpenShuttersToReveal(const Sifteo::AssetImage &image);
        void CloseShutters();
				
		void DrawVaultDoorsClosed();

        void Image(const Sifteo::AssetImage &image);
        void Image(const Sifteo::AssetImage &image, Vector2<int> pos, int frame=0);
        void Image(const Sifteo::AssetImage *image, Vector2<int> coord, Vector2<int> offset, Vector2<int> size);
        void Image(const Sifteo::PinnedAssetImage *image, Vector2<int> coord, int frame=0);
        void ClipImage(const Sifteo::AssetImage *image, Vector2<int> pos);
        void ClipImage(const Sifteo::PinnedAssetImage *image, Vector2<int> pos, int frame = 0);
        void FillArea(const Sifteo::AssetImage *image, Vector2<int> pos, Vector2<int> size);
	
        void DrawFraction(Fraction f, Vector2<int> pos);
        //void DrawDecimal(float d, Vec2 pos);
        void DrawString(const char *string, Vector2<int> center);

        void EnableTextOverlay(const char *text, int yTop, int ySize, int br, int bg, int bb, int fr, int fg, int fb);
        void DisableTextOverlay();
        bool IsTextOverlayEnabled();

        void DispatchOnCubeShake(TotalsCube *c);
        void DispatchOnCubeTouch(TotalsCube *c, bool touching);

//	private:

		// for these methods, 0 <= offset <= 32
        void DrawVaultDoorsOpenStep1(int offset);
        void DrawVaultDoorsOpenStep2(int offset);
	};

}
