/* The MIT License:

Copyright (c) 2008-2012 Ivan Gagis <igagis@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE. */

// Home page: http://ting.googlecode.com



/**
 * @file math.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Math utilities.
 */


#pragma once

#include <cmath>



namespace ting{
namespace math{

/**
 * @brief Get sign of a number.
 * Template function which returns the sign of a number.
 * General implementation of this template is as easy as:
 * @code
 * template <typename T> inline T Sign(T n){
 *     return n > 0 ? (1) : (-1);
 * }
 * @endcode
 * @param n - number to get sign of.
 * @return -1 if the argument is negative.
 * @return 1 if the number is positive.
 */
template <typename T> inline T Sign(T n)throw(){
	return n < 0 ? T(-1) : T(1);
}



/**
 * @brief Get absolute value of a number.
 * General implementation of this function is as follows:
 * @code
 * template <typename T> inline T Abs(T n){
 *     return n > 0 ? n : (-n);
 * }
 * @endcode
 * @param n - number to get absolute value of.
 * @return absolute value of the passed argument.
 */
template <typename T> inline T Abs(T n)throw(){
	return n < 0 ? (-n) : (n);
}



/**
 * @brief Get number Pi.
 * @return number Pi.
 */
template <typename T> inline T DPi()throw(){
	return T(3.14159265358979323846264338327950288);
}



/**
 * @brief Get 2 * Pi.
 * @return 2 * Pi.
 */
template <typename T> inline T D2Pi()throw(){
	return T(2) * DPi<T>();
}



/**
 * @brief Get natural logarithm of 2, i.e. ln(2).
 * @return natural logarithm of 2.
 */
template <typename T> inline T DLnOf2()throw(){
	return T(0.693147181);
}



//Power functions

/**
 * @brief Calculate x^2.
 * @param x - value.
 * @return x * x.
 */
template <typename T> inline T Pow2(T x)throw(){
	return x * x;
}

/**
 * @brief Calculate x^3.
 */
template <typename T> inline T Pow3(T x)throw(){
	return Pow2(x) * x;
}

/**
 * @brief Calculate x^4.
 */
template <typename T> inline T Pow4(T x)throw(){
	return Pow2(Pow2(x));
}

/**
 * @brief Calculate x^5.
 */
template <typename T> inline T Pow5(T x)throw(){
	return Pow4(x) * x;
}

/**
 * @brief Calculate x^6.
 */
template <typename T> inline T Pow6(T x)throw(){
	return Pow2(Pow3(x));
}



/**
 * @brief Calculates x^p.
 * @param x - value
 * @param p - power
 * @return x^p
 */
template <typename T> inline T Pow(T x, T p)throw(){
	return x.Pow(p);
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Pow<float>(float x, float p)throw(){
	return ::pow(x, p);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Pow<double>(double x, double p)throw(){
	return ::pow(x, p);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Pow<long double>(long double x, long double p)throw(){
	return ::pow(x, p);
}
#endif



/**
 * @brief Calculate sine of an angle.
 */
template <typename T> inline T Sin(T x)throw(){
	return x.Sin();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Sin<float>(float x)throw(){
	return ::sin(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Sin<double>(double x)throw(){
	return ::sin(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Sin<long double>(long double x)throw(){
	return ::sin(x);
}
#endif



/**
 * @brief Calculate cosine of an angle.
 */
template <typename T> inline T Cos(T x)throw(){
	return x.Cos();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Cos<float>(float x)throw(){
	return ::cos(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Cos<double>(double x)throw(){
	return ::cos(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Cos<long double>(long double x)throw(){
	return ::cos(x);
}
#endif



/**
 * @brief Calculate tangent of a number.
 */
template <typename T> inline T Tan(T x)throw(){
	return x.Tan();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Tan<float>(float x)throw(){
	return ::tan(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Tan<double>(double x)throw(){
	return ::tan(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Tan<long double>(long double x)throw(){
	return ::tan(x);
}
#endif



/**
 * @brief Calculate arcsine of a number.
 */
template <typename T> inline T Asin(T x)throw(){
	return x.Asin();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Asin<float>(float x)throw(){
	return ::asin(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Asin<double>(double x)throw(){
	return ::asin(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Asin<long double>(long double x)throw(){
	return ::asin(x);
}
#endif



/**
 * @brief Calculate arccosine of a number.
 */
template <typename T> inline T Acos(T x)throw(){
	return x.Acos();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Acos<float>(float x)throw(){
	return ::acos(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Acos<double>(double x)throw(){
	return ::acos(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Acos<long double>(long double x)throw(){
	return ::acos(x);
}
#endif



/**
 * @brief Calculate arctangent of a number.
 */
template <typename T> inline T Atan(T x)throw(){
	return x.Atan();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Atan<float>(float x)throw(){
	return ::atan(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Atan<double>(double x)throw(){
	return ::atan(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Atan<long double>(long double x)throw(){
	return ::atan(x);
}
#endif



/**
 * @brief Calculate square root of a number.
 */
template <typename T> inline T Sqrt(T x)throw(){
	return x.Sqrt();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Sqrt<float>(float x)throw(){
	return ::sqrt(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Sqrt<double>(double x)throw(){
	return ::sqrt(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Sqrt<long double>(long double x)throw(){
	return ::sqrt(x);
}
#endif



/**
 * @brief Calculate e^x.
 */
template <typename T> inline T Exp(T x)throw(){
	return x.Exp();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Exp<float>(float x)throw(){
	return ::exp(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Exp<double>(double x)throw(){
	return ::exp(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Exp<long double>(long double x)throw(){
	return ::exp(x);
}
#endif



/**
 * @brief Calculate ln(x).
 * Calculate natural logarithm of x.
 */
template <typename T> inline T Ln(T x)throw(){
	return x.Ln();
}

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline float Ln<float>(float x)throw(){
	return ::log(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline double Ln<double>(double x)throw(){
	return ::log(x);
}
#endif

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
template <> inline long double Ln<long double>(long double x)throw(){
	return ::log(x);
}
#endif



/**
 * @brief Calculate cubic root of a number.
 */
template <typename T> inline T CubicRoot(T x)throw(){
	if(x > 0){
		return Exp<T>(Ln<T>(x) / 3);
	}
	
	if(x == 0){
		return 0;
	}
	
	return -Exp<T>(Ln<T>(-x) / 3);
}



/**
 * @brief Calculate argument of a complex number.
 * Any complex number
 *     C = x + i * y
 * can be represented in the form
 *     C = |C| * exp(i * arg)
 * where 'arg' is the argument of a complex number.
 * @param x - real part of a complex number.
 * @param y - imaginary part of a complex number.
 * @return argument of a complex number.
 */
template <typename T> inline T Arg(T x, T y)throw(){
	T a;
	if(x == 0){
		a = DPi<T>() / 2;
	}else if(x > 0){
		a = T(Atan(Abs(y / x)));
	}else{
		a = DPi<T>() - T(Atan(Abs(y / x)));
	}

	if(y >= 0){
		return a;
	}else{
		return -a;
	}
}


}//~namespace math
}//~namespace ting
