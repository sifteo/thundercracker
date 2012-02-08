#pragma once

#include "sifteo.h"
#include "coroutine.h"
#include "View.h"

namespace TotalsGame
{

	

	class TotalsCube: public Sifteo::Cube
	{
		CORO_PARAMS;
		float t;

		View *view;

	public:

		class EventHandler
		{
		public:
			virtual void OnCubeShake() {}
			virtual void OnCubeTouched() {}
		};

		TotalsCube();
		static const float kTransitionTime = 0.1f;

		void SetView(View *v) {delete view;view = v;}
		View *GetView(void) {return view;}

		EventHandler *eventHandler;

		float OpenShutters(const AssetImage *image);
		float CloseShutters(const AssetImage *image);
				
		void DrawVaultDoorsClosed();

		void Image(const AssetImage *image);
		void Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size);
	
	private:

		// for these methods, 0 <= offset <= 32
		void DrawVaultDoorsOpenStep1(int offset, const AssetImage *innerImage);
		void DrawVaultDoorsOpenStep2(int offset, const AssetImage *innerImage);
	};

}