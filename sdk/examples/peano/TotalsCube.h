#pragma once

#include "sifteo.h"
#include "View.h"
#include "Fraction.h"

using namespace Sifteo;

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

        Int2 GetTilt();

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
        void Image(const Sifteo::AssetImage &image, Int2 pos, int frame=0);
        void Image(const Sifteo::AssetImage *image, const Int2 &coord, const Int2 &offset, const Int2 &size);
        void Image(const Sifteo::PinnedAssetImage *image, Int2 coord, int frame=0);
        void ClipImage(const Sifteo::AssetImage *image, Int2 pos);
        void ClipImage(const Sifteo::PinnedAssetImage *image, Int2 pos, int frame = 0);
        void FillArea(const Sifteo::AssetImage *image, Int2 pos, Int2 size);
	
        void DrawFraction(Fraction f, Int2 pos);
        //void DrawDecimal(float d, vec pos);
        void DrawString(const char *string, Int2 center);

        void EnableTextOverlay(const char *text, int yTop, int ySize, int fg[3], int bg[3]);
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
