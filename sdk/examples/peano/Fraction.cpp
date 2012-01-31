#include "Fraction.h"

namespace TotalsGame {

	Fraction::Fraction(int n, int d=1)
	{
		nu = n;
		de = d;
	}

	Fraction::Fraction(int x)
	{
		Fraction(x, 1);
	}

	Fraction Fraction::operator+(const Fraction f)
	{
		return Fraction(nu * f.de + f.nu * de, de * f.de);
	}

	Fraction Fraction::operator-(const Fraction f)
	{
		return Fraction(nu * f.de - f.nu * de, de * f.de);
	}

	Fraction Fraction::operator*(const Fraction f)
	{
		return Fraction(nu * f.nu, de * f.de);
	}

	Fraction Fraction::operator/(const Fraction f)
	{
		return Fraction(nu * f.de, de * f.nu);
	}

	bool Fraction::IsNan()
	{
		Reduce();
		return de == 0;
	}

	bool Fraction::IsZero()
	{
		Reduce();
		return nu == 0 && de != 0;
	}

	bool Fraction::IsNonZero()
	{
		Reduce();
		return nu != 0 && de != 0;
	}

	void Fraction::Reduce()
	{
		if (nu == 0) { de = 1; return; }
		if (de == 0) { nu = 0; return; }
		int gcd = GCD(nu, de);
		if (gcd == 0) { return; }
		nu /= gcd;
		de /= gcd;
		if (de < 0) {
			nu *= -1;
			de *= -1;
		}
	}

	bool Fraction::operator==(Fraction &f)
	{
		Reduce();
		f.Reduce();
		return nu == f.nu && de == f.de;
	}

	void Fraction::ToString(char *s, int length)
	{
		char numString[8];
		char denString[8];

		snprintf(numString, 8, "%d", nu);
		snprintf(denString, 8, "%d", de);
		snprintf(s, length, "%s/%s", numString, denString);
	}

	static int Fraction::GCD(int a, int b) 
	{
		int c;
		if (a < b) {
			c = a;
			a = b;
			b = c;
			if (b == 0) { return 0; }
		}
		while((c = a%b) != 0) {
			a = b;
			b = c;
			if (b == 0) { return 0; }
		}
		return b;
	}

}

