/* The MIT License:

Copyright (c) 2009-2012 Ivan Gagis <igagis@gmail.com>

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
 * @file utils.hpp
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Utility functions and classes.
 */

#pragma once

//#ifdef _MSC_VER //If Microsoft C++ compiler
//#pragma warning(disable:4290)
//#endif

#include <vector>

#include "debug.hpp" //debugging facilities
#include "types.hpp"

//define macro used to align structures in memory
#ifdef _MSC_VER //If Microsoft C++ compiler
#define M_DECLARE_ALIGNED(x)
#define M_DECLARE_ALIGNED_MSVC(x) __declspec(align(x))

#elif defined(__GNUG__)//GNU g++ compiler
#define M_DECLARE_ALIGNED(x) __attribute__ ((aligned(x)))
#define M_DECLARE_ALIGNED_MSVC(x)

#else
#error "unknown compiler"
#endif


namespace ting{
namespace util{



/**
 * @brief Clamp value top.
 * This inline template function can be used to clamp the top of the value.
 * Example:
 * @code
 * int a = 30;
 *
 * //Clamp to 40. Value of 'a' remains unchanged,
 * //since it is already less than 40.
 * ting::ClampTop(a, 40);
 * std::cout << a << std::endl;
 *
 * //Clamp to 27. Value of 'a' is changed to 27,
 * //since it is 30 which is greater than 27.
 * ting::ClampTop(a, 27);
 * std::cout << a << std::endl;
 * @endcode
 * As a result, this will print:
 * @code
 * 30
 * 27
 * @endcode
 * @param v - reference to the value which top is to be clamped.
 * @param top - value to clamp the top to.
 */
template <class T> inline void ClampTop(T& v, const T top)throw(){
	if(v > top){
		v = top;
	}
}


/**
 * @brief Clamp value bottom.
 * Usage is analogous to ting::ClampTop.
 * @param v - reference to the value which bottom is to be clamped.
 * @param bottom - value to clamp the bottom to.
 */
template <class T> inline void ClampBottom(T& v, const T bottom)throw(){
	if(v < bottom){
		v = bottom;
	}
}



/**
 * @brief serialize 16 bit value, little-endian.
 * Serialize 16 bit value, less significant byte first.
 * @param value - the value.
 * @param out_buf - pointer to the 2 byte buffer where the result will be placed.
 */
inline void Serialize16LE(u16 value, u8* out_buf)throw(){
	out_buf[0] = value & 0xff;
	out_buf[1] = value >> 8;
}



/**
 * @brief serialize 32 bit value, little-endian.
 * Serialize 32 bit value, less significant byte first.
 * @param value - the value.
 * @param out_buf - pointer to the 4 byte buffer where the result will be placed.
 */
inline void Serialize32LE(u32 value, u8* out_buf)throw(){
	*out_buf = u8(value & 0xff);
	++out_buf;
	*out_buf = u8((value >> 8) & 0xff);
	++out_buf;
	*out_buf = u8((value >> 16) & 0xff);
	++out_buf;
	*out_buf = u8((value >> 24) & 0xff);
}



/**
 * @brief de-serialize 16 bit value, little-endian.
 * De-serialize 16 bit value from the sequence of bytes. Assume that less significant
 * byte goes first in the input byte sequence.
 * @param buf - pointer to buffer containing 2 bytes to convert from little-endian format.
 * @return 16 bit unsigned integer converted from little-endian byte order to native byte order.
 */
inline u16 Deserialize16LE(const u8* buf)throw(){
	u16 ret;

	//assume little-endian
	ret = u16(*buf);
	++buf;
	ret |= ((u16(*buf)) << 8);

	return ret;
}



/**
 * @brief de-serialize 32 bit value, little-endian.
 * De-serialize 32 bit value from the sequence of bytes. Assume that less significant
 * byte goes first in the input byte sequence.
 * @param buf - pointer to buffer containing 4 bytes to convert from little-endian format.
 * @return 32 bit unsigned integer converted from little-endian byte order to native byte order.
 */
inline u32 Deserialize32LE(const u8* buf)throw(){
	u32 ret;

	//assume little-endian
	ret = u32(*buf);
	++buf;
	ret |= ((u32(*buf)) << 8);
	++buf;
	ret |= ((u32(*buf)) << 16);
	++buf;
	ret |= ((u32(*buf)) << 24);

	return ret;
}



/**
 * @brief serialize 16 bit value, big-endian.
 * Serialize 16 bit value, most significant byte first.
 * @param value - the value.
 * @param out_buf - pointer to the 2 byte buffer where the result will be placed.
 */
inline void Serialize16BE(u16 value, u8* out_buf)throw(){
	out_buf[0] = value >> 8;
	out_buf[1] = value & 0xff;
}



/**
 * @brief serialize 32 bit value, big-endian.
 * Serialize 32 bit value, most significant byte first.
 * @param value - the value.
 * @param out_buf - pointer to the 4 byte buffer where the result will be placed.
 */
inline void Serialize32BE(u32 value, u8* out_buf)throw(){
	*out_buf = u8((value >> 24) & 0xff);
	++out_buf;
	*out_buf = u8((value >> 16) & 0xff);
	++out_buf;
	*out_buf = u8((value >> 8) & 0xff);
	++out_buf;
	*out_buf = u8(value & 0xff);
}



/**
 * @brief de-serialize 16 bit value, big-endian.
 * De-serialize 16 bit value from the sequence of bytes. Assume that most significant
 * byte goes first in the input byte sequence.
 * @param buf - pointer to buffer containing 2 bytes to convert from big-endian format.
 * @return 16 bit unsigned integer converted from big-endian byte order to native byte order.
 */
inline u16 Deserialize16BE(const u8* buf)throw(){
	u16 ret;

	//assume big-endian
	ret = ((u16(*buf)) << 8);
	++buf;
	ret |= u16(*buf);

	return ret;
}



/**
 * @brief de-serialize 32 bit value, big-endian.
 * De-serialize 32 bit value from the sequence of bytes. Assume that most significant
 * byte goes first in the input byte sequence.
 * @param buf - pointer to buffer containing 4 bytes to convert from big-endian format.
 * @return 32 bit unsigned integer converted from big-endian byte order to native byte order.
 */
inline u32 Deserialize32BE(const u8* buf)throw(){
	u32 ret;

	//assume big-endian
	ret = ((u32(*buf)) << 24);
	++buf;
	ret |= ((u32(*buf)) << 16);
	++buf;
	ret |= ((u32(*buf)) << 8);
	++buf;
	ret |= u32(*buf);

	return ret;
}



}//~namespace
}//~namespace ting
