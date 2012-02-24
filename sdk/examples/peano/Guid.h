#pragma once
#include "sifteo.h"

namespace TotalsGame
{

	class Guid
	{
	public:
        static const Guid Empty;

        Guid();
        Guid(const uint32_t *theInts);

        bool operator==(const Guid &right) const;
        bool operator!=(const Guid &right) const;

	private:
        uint32_t guid[4];
	};


}
