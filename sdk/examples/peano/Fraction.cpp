#include "Fraction.h"
#include "sifteo.h"

namespace TotalsGame {

	Fraction::Fraction(int n, int d)
	{
		nu = n;
		de = d;		
	}

	
	Fraction Fraction::operator+(const Fraction &f)
	{
		return Fraction(nu * f.de + f.nu * de, de * f.de);
	}

	Fraction Fraction::operator-(const Fraction &f)
	{
		return Fraction(nu * f.de - f.nu * de, de * f.de);
	}

	Fraction Fraction::operator*(const Fraction &f)
	{
		return Fraction(nu * f.nu, de * f.de);
	}

	Fraction Fraction::operator/(const Fraction &f)
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

	bool Fraction::operator==(const Fraction &f)
	{
		Fraction t = f;
		Reduce();
		t.Reduce();
		return nu == t.nu && de == t.de;
	}

	bool Fraction::operator!=(const Fraction &f)
	{
		Fraction t = f;
		Reduce();
		t.Reduce();
		return nu != t.nu || de != t.de;
	}

	int Fraction::GCD(int a, int b) 
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

