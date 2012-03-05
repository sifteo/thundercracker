#pragma once

#include "sifteo.h"
#include "ObjectPool.h"
#include <stddef.h>

namespace TotalsGame
{
	class TotalsCube;

	class View
	{	
		TotalsCube *mCube;
        int mLockCount;

	public:
		View(TotalsCube *_cube);

		virtual void Paint() {}
		virtual void Update() {}
		virtual void DidAttachToCube(TotalsCube *c) {}
		virtual void WillDetachFromCube(TotalsCube *c) {}
		virtual ~View() {};

		static void PaintViews(TotalsCube *cubes, int numCubes);

		void SetCube(TotalsCube *c);
		TotalsCube *GetCube();

        //for placement new
        void* operator new (size_t size, void* ptr) throw() {return ptr;}
        void operator delete(void *ptr) {}
	};

}
