#pragma once

namespace TotalsGame
{

	class Guid
	{
	public:
		Guid();

        bool operator==(const Guid &right);
		bool operator!=(const Guid &right);

		static const Guid Empty;

	private:
		unsigned long guid[4];
	};


}