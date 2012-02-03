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

	
	bool Guid::operator!=(const Guid &right)
	{
		return guid[0] != right.guid[0]
			|| guid[1] != right.guid[1]
			|| guid[2] != right.guid[2]
			|| guid[3] != right.guid[3];
	}

}