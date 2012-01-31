#pragma once

namespace TotalsGame {

	class Fraction
	{
	public:
		int nu;
		int de;

		Fraction(int n, int d=1);
		Fraction(int x);

		Fraction operator+(const Fraction f);
		Fraction operator-(const Fraction f);
		Fraction operator*(const Fraction f);
		Fraction operator/(const Fraction f);
		bool operator==(Fraction &f);

		bool IsNan();
		bool IsZero();
		bool IsNonZero();
		void Reduce();

		void ToString(char *s, int length);

		static int GCD(int a, int b);
	};
}

