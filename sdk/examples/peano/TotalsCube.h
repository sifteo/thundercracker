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
		static const float kTransitionTime = 1.1f;

		void OpenShutters(const char *image);
		void CloseShutters();

		bool AreShuttersOpen();
		bool AreShuttersClosed();
				
		void DrawVaultDoorsClosed();

		void Image(AssetImage *image);
	
	private:

		// for these methods, 0 <= offset <= 32
		void DrawVaultDoorsOpenStep1(int offset, const char*);
		void DrawVaultDoorsOpenStep2(int offset, const char*);
	};

}