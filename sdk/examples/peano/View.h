#pragma once

#include "sifteo.h"
#include "ObjectPool.h"

namespace TotalsGame
{
	class TotalsCube;

	class View
	{	
		DECLARE_POOL(View, 0)		

	public:
		View(TotalsCube *_cube);

		virtual void Paint() {}
		virtual void Update(float dt) {}
		virtual ~View() {};

		static void PaintViews(TotalsCube *cubes, int numCubes);

		TotalsCube *cube;
	};

}