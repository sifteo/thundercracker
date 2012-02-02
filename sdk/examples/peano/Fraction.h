#pragma once

namespace TotalsGame 
{

	class Fraction
	{
	public:
		int nu;
		int de;

		Fraction(int n, int d);
		Fraction(int x);
		Fraction();

		Fraction operator+(const Fraction f);
		Fraction operator-(const Fraction f);
		Fraction operator*(const Fraction f);
		Fraction operator/(const Fraction f);
		bool operator==(const Fraction &f);
		bool operator!=(const Fraction &f);

		bool IsNan();
		bool IsZero();
		bool IsNonZero();
		void Reduce();

		void ToString(char *s, int length);

		static int GCD(int a, int b);
	};
}

