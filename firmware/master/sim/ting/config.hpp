/* The MIT License:

Copyright (c) 2011-2012 Ivan Gagis <igagis@gmail.com>

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
 * @brief Environment configuration definitions.
 */

#pragma once



//====================================================|
//            Compiler definitions                    |
//                                                    |

#define M_COMPILER_UNKNOWN                            0
#define M_COMPILER_GCC                                1
#define M_COMPILER_MSVC                               2

#if defined(__GNUC__) || defined(__GNUG__)
	#define M_COMPILER M_COMPILER_GCC
#elif defined(_MSC_VER)
	#define M_COMPILER M_COMPILER_MSVC
#else
	#define M_COMPILER M_COMPILER_UNKNOWN
#endif



//====================================================|
//            CPU architecture definitions            |
//                                                    |

#define M_CPU_UNKNOWN                                 0
#define M_CPU_X86                                     1
#define M_CPU_X86_64                                  2
#define M_CPU_ARM                                     3


#if M_COMPILER == M_COMPILER_GCC
	#if defined(__i386__) //__i386__ is defined for any x86 processor
		#define M_CPU M_CPU_X86
		
		#if defined(__i686__)
			#define M_CPU_VERSION 6
		#elif defined(__i586__)
			#define M_CPU_VERSION 5
		#elif defined(__i486__)
			#define M_CPU_VERSION 4
		#else
			#define M_CPU_VERSION 3
		#endif
		
	#elif defined(__x86_64__)
		#define M_CPU M_CPU_X86_64
		
		#define M_CPU_VERSION 0
		
	#elif defined(__arm__)
		#define M_CPU M_CPU_ARM
		
		#if defined(__thumb2__) //this macro is defined when targeting only thumb-2
			#define M_CPU_ARM_THUMB 2
		#elif defined(__thumb__) //this macro is defined when targeting any, thumb-1 or thumb-2
			#define M_CPU_ARM_THUMB 1
		#endif
		
		#if defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__)
			#define M_CPU_VERSION 7
		#elif defined(__ARM_ARCH_6T2__) || defined(__ARM_ARCH_6ZK__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6__)
			#define M_CPU_VERSION 6
		#elif defined(__ARM_ARCH_5TEJ__) || defined(__ARM_ARCH_5TE__) || defined(__ARM_ARCH_5T__) || defined(__ARM_ARCH_5E__) || defined(__ARM_ARCH_5__)
			#define M_CPU_VERSION 5
		#elif defined(__ARM_ARCH_4T__) || defined(__ARM_ARCH_4__)
			#define M_CPU_VERSION 4
		#else
			#define M_CPU_VERSION 0
		#endif

	#else
		#define M_CPU M_CPU_UNKNOWN
		#define M_CPU_VERSION 0
	#endif

#elif M_COMPILER == M_COMPILER_MSVC
	#if defined(_M_IX86)
		#define M_CPU M_CPU_X86
		
		#if _M_IX86 == 600
			#define M_CPU_VERSION 6
		#elif _M_IX86 == 500
			#define M_CPU_VERSION 5
		#elif _M_IX86 == 400
			#define M_CPU_VERSION 4
		#else
			#define M_CPU_VERSION 3
		#endif
		
	#elif defined(_M_AMD64) || defined(_M_X64)
		#define M_CPU M_CPU_X86_64
		#define M_CPU_VERSION 0
		
	#else
		#define M_CPU M_CPU_UNKNOWN
		#define M_CPU_VERSION 0
	#endif
#else
	#define M_CPU M_CPU_UNKNOWN
	#define M_CPU_VERSION 0
#endif



//======================================|
//            OS definitions            |
//                                      |

#define M_OS_UNKNOWN                    0
#define M_OS_LINUX                      1
#define M_OS_WIN64                      2
#define M_OS_WIN32                      3
#define M_OS_MACOSX                     4
#define M_OS_SOLARIS                    5

#if defined(__linux__)
	#define M_OS M_OS_LINUX
#elif defined(_WIN64) //check for win64 should go before check for win32 because WIN32 macro is defined for both win32 and win64
	#define M_OS M_OS_WIN64
#elif defined(WIN32)
	#define M_OS M_OS_WIN32
#elif defined(__APPLE__)
	#define M_OS M_OS_MACOSX
#elif defined(sun) || defined(__sun)
	#define M_OS M_OS_SOLARIS
#else
	#define M_OS M_OS_UNKNOWN
#endif
