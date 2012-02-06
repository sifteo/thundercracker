#pragma once

#include "TotalsCube.h"

namespace TotalsGame
{
	class View
	{		
	public:
		View(TotalsCube *_cube)
		{
			cube = _cube;	
		}

		virtual void Paint() {}
		virtual void Update(float dt) {}
		//virtual ~View() {};

		TotalsCube *cube;
	};

}