#pragma once

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

		void ToString(char *s, int length);

		static int GCD(int a, int b);

		int nu;
		int de;
	};
}

