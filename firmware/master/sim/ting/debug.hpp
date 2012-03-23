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
 * @author Ivan Gagis <igagis@gmail.com>
 * @brief Debug utilities.
 */

#pragma once

#if defined(__SYMBIAN32__)
#include <e32std.h>

#elif defined(__ANDROID__)
	#undef NDEBUG // we want assertions to work, if we don't undef NDEBUG the assertions will be translated to nothing
	#include <cassert>

	#include <sstream>
	#include <android/log.h>

#else //assume more or less standard system
#include <sstream>
#include <iostream>
#include <fstream>
#include <typeinfo>
#include <cassert>

#endif

#if defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif

//
//
//  Logging definitions
//
//

#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
namespace ting{
namespace ting_debug{
#if defined(__SYMBIAN32__)
#elif defined(__ANDROID__)
#else
inline std::ofstream& DebugLogger(){
	//this allows to make debug output even if main() is not called yet and even if
	//standard std::cout object is not created since static global variables initialization
	//order is undetermined in C++ if these variables are located in separate cpp files!
	static std::ofstream* logger = new std::ofstream("output.log");
	return *logger;
}
#endif
}//~namespace ting_debug
}//~namespace ting
#endif //~M_DOXYGEN_DONT_EXTRACT //for doxygen



#if defined(__SYMBIAN32__)
#define LOG_ALWAYS(x)
#define TRACE_ALWAYS(x)

#elif defined(__ANDROID__)
#define TRACE_ALWAYS(x) \
	{ \
		std::stringstream ss; \
		ss x; \
		__android_log_print(ANDROID_LOG_INFO, "ting_debug", ss.str().c_str()); \
	}
#define LOG_ALWAYS(x) //logging is not supported on Android, yet.

#else
#define LOG_ALWAYS(x) ting::ting_debug::DebugLogger() x; ting::ting_debug::DebugLogger().flush();
#define TRACE_ALWAYS(x) std::cout x; std::cout.flush();

#endif

#define TRACE_AND_LOG_ALWAYS(x) LOG_ALWAYS(x) TRACE_ALWAYS(x)



#ifdef DEBUG

#define LOG(x) LOG_ALWAYS(x)
#define TRACE(x) TRACE_ALWAYS(x)
#define TRACE_AND_LOG(x) TRACE_AND_LOG_ALWAYS(x)

#define LOG_IF_TRUE(x, y) if(x){ LOG(y) }

#define DEBUG_CODE(x) x

#else//#ifdef DEBUG

#define LOG(x)
#define TRACE(x)
#define TRACE_AND_LOG(x)
#define LOG_IF_TRUE(x, y)
#define DEBUG_CODE(x)

#endif//~#ifdef DEBUG



//
//
//  Assertion definitions
//
//
namespace ting{
namespace ting_debug{
inline void LogAssert(const char* msg, const char* file, int line){
	TRACE_AND_LOG_ALWAYS(<< "[!!!fatal] Assertion failed at:\n\t"<< file << ":" << line << "| " << msg << std::endl)
}
}
}
#if defined(__SYMBIAN32__)
#define ASSERT_INFO_ALWAYS(x, y) __ASSERT_ALWAYS((x), User::Panic(_L("ASSERTION FAILED!"),3));

#else //Assume system supporting standard assert() (including Android)

#define ASSERT_INFO_ALWAYS(x, y) if(!(x)){ \
						std::stringstream ss; \
						ss << y; \
						ting::ting_debug::LogAssert(ss.str().c_str(), __FILE__, __LINE__); \
						assert(false); \
					}

#endif

#define ASSERT_ALWAYS(x) ASSERT_INFO_ALWAYS((x), "no additional info")



#ifdef DEBUG
#define ASSERT_INFO(x, y) ASSERT_INFO_ALWAYS((x), y)
#define ASSERT(x) ASSERT_ALWAYS(x)

#if defined(__SYMBIAN32__)
#define ASS(x) (x)
#define ASSCOND(x, cond) (x)

#else //Assume system supporting standard assert() (including Android)
#define ASS(x) ( (x) ? (x) : (ting::ting_debug::LogAssert("ASS() assertion macro", __FILE__, __LINE__), (assert(false)), (x)) )
#define ASSCOND(x, cond) ( ((x) cond) ? (x) : (ting::ting_debug::LogAssert("ASS() assertion macro", __FILE__, __LINE__), (assert(false)), (x)) )

#endif

#else //No DEBUG macro defined
#define ASSERT_INFO(x, y)
#define ASSERT(x)
#define ASS(x) (x)
#define ASSCOND(x, cond) (x)

#endif//~#ifdef DEBUG

//==================
//=  Static assert =
//==================
#ifndef M_DOXYGEN_DONT_EXTRACT //for doxygen
namespace ting{
namespace ting_debug{
template <bool b> struct C_StaticAssert{
	virtual void STATIC_ASSERTION_FAILED() = 0;
	virtual ~C_StaticAssert(){};
};
template <> struct C_StaticAssert<true>{};
}//~namespace ting_debug
}//~namespace ting
#define M_STATIC_ASSERT_II(x, l, c) struct C_StaticAssertInst_##l##_##c{ \
	ting::ting_debug::C_StaticAssert<x> STATIC_ASSERTION_FAILED; \
};
#define M_STATIC_ASSERT_I(x, l, c) M_STATIC_ASSERT_II(x, l, c)
#endif //~M_DOXYGEN_DONT_EXTRACT //for doxygen

#if defined(__GNUG__) || (_MSC_VER >= 7100) //__COUNTER__ macro is only supported in these compilers
#define STATIC_ASSERT(x) M_STATIC_ASSERT_I(x, __LINE__, __COUNTER__)
#else
#define STATIC_ASSERT(x)
#endif

