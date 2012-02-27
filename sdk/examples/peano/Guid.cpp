#if 0

#include "Guid.h"

namespace TotalsGame
{
    const Guid Guid::Empty;

    Guid::Guid()
    {
        guid[0] = 0;
        guid[1] = 0;
        guid[2] = 0;
        guid[3] = 0;
    }

    Guid::Guid(const uint32_t *theInts)
	{
        guid[0] = theInts[0];
        guid[1] = theInts[1];
        guid[2] = theInts[2];
        guid[3] = theInts[3];
	}

    bool Guid::operator==(const Guid &right) const
    {
        return guid[0] == right.guid[0]
            && guid[1] == right.guid[1]
            && guid[2] == right.guid[2]
            && guid[3] == right.guid[3];
    }
	
    bool Guid::operator!=(const Guid &right) const
	{
		return guid[0] != right.guid[0]
			|| guid[1] != right.guid[1]
			|| guid[2] != right.guid[2]
			|| guid[3] != right.guid[3];
	}

}

#endif