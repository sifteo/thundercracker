#pragma once

#include "sifteo.h"
#include "coroutine.h"

namespace TotalsGame
{
	class TotalsCube: public Sifteo::Cube
	{
		CORO_PARAMS
		float t;

		bool shuttersClosed;
		bool shuttersOpen;

	public:
		TotalsCube();
		static const float kTransitionTime = 0.1f;

		void OpenShutters(const AssetImage *image);
		void CloseShutters(const AssetImage *image);

		bool AreShuttersOpen();
		bool AreShuttersClosed();
				
		void DrawVaultDoorsClosed();

		void Image(const AssetImage *image);
		void Image(const AssetImage *image, const Vec2 &coord, const Vec2 &offset, const Vec2 &size);
	
	private:

		// for these methods, 0 <= offset <= 32
		void DrawVaultDoorsOpenStep1(int offset, const AssetImage *innerImage);
		void DrawVaultDoorsOpenStep2(int offset, const AssetImage *innerImage);
	};

}