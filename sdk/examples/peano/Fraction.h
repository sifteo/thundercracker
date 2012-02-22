#pragma once
#include "sifteo.h"

namespace TotalsGame 
{

	class Fraction
	{
	public:


		Fraction(int n = 0, int d = 1);

		Fraction operator+(const Fraction &f);
		Fraction operator-(const Fraction &f);
		Fraction operator*(const Fraction &f);
		Fraction operator/(const Fraction &f);
		bool operator==(const Fraction &f);
		bool operator!=(const Fraction &f);

        operator float(){return nu/(float)de;}

		bool IsNan();
		bool IsZero();
		bool IsNonZero();
		void Reduce();

        template <unsigned int capacity>
        void ToString(String<capacity> *output)
        {
            *output << nu;

            if(de != 1)
            {
                *output << "/" << de;
            }
        }

		static int GCD(int a, int b);

		int nu;
		int de;
	};
}

