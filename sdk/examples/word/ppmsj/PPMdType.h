/****************************************************************************
 *  This file is part of PPMs project                                       *
 *  Written and distributed to public domain by Dmitry Shkarin 2004, 2006   *
 *  Contents: compilation parameters and miscelaneous definitions           *
 *  Comments: system & compiler dependent file                              *
 ****************************************************************************/
#if !defined(_PPMDTYPE_H_)
#define _PPMDTYPE_H_
#include <stdio.h>

#define _WINDOWS_ENVIRONMENT_
//#define _DOS32_ENVIRONMENT_
//#define _POSIX_ENVIRONMENT_
//#define _UNKNOWN_ENVIRONMENT_
#if defined(_WINDOWS_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) != 1
#error Only one environment must be defined
#endif /* defined(_WINDOWS_ENVIRONMENT_)+defined(_DOS32_ENVIRONMENT_)+defined(_POSIX_ENVIRONMENT_)+defined(_UNKNOWN_ENVIRONMENT_) != 1 */

#if defined(_WINDOWS_ENVIRONMENT_)
#include <windows.h>
#else /* _DOS32_ENVIRONMENT_ || _POSIX_ENVIRONMENT_ || _UNKNOWN_ENVIRONMENT_ */
typedef int   BOOL;
#define FALSE 0
#define TRUE  1
typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned int  DWORD;
typedef unsigned int   UINT;
#endif /* defined(_WINDOWS_ENVIRONMENT_)  */

//#define _USE_64BIT_POINTERS

/* _USE_THREAD_KEYWORD macro must be defined at compilation for creation    *
 * of multithreading applications. Some compilers generate correct code     *
 * with the use of standard '__thread' keyword (GNU C), some others use it  *
 * in non-standard way (BorlandC) and some use __declspec(thread) keyword   *
 * (IntelC, VisualC).                                                       */
//#define _USE_THREAD_KEYWORD
#if defined(_USE_THREAD_KEYWORD)
#if defined(_MSC_VER)
#define _THREAD
#define _THREAD1 __declspec(thread)
#elif defined(__GNUC__)
#define _THREAD
#define _THREAD1 __thread
#else /* __BORLANDC__ */
#define _THREAD __thread
#define _THREAD1
#endif /* defined(_MSC_VER) */
#else
#define _THREAD
#define _THREAD1
#endif /* defined(_USE_THREAD_KEYWORD) */

#if !defined(_MEM_CONFIG)
#define _MEM_CONFIG 1024
#elif ((_MEM_CONFIG > 512 || _MEM_CONFIG < 64) && _MEM_CONFIG != 1024)
#error _MEM_CONFIG must be only [64-512] or 1024
#endif /* !defined(_MEM_CONFIG) */

const DWORD PPMdSignature=0x91ACAF8F;
#if (_MEM_CONFIG == 1024)
enum {PROG_VAR='J', DEFAULT_O=5, MAX_O=8};  /* maximum allowed model order  */
#else
enum {PROG_VAR='J', DEFAULT_O=4, MAX_O=6};
#endif /* _MEM_CONFIG == 1024 */

#define _USE_PREFETCHING                    /* for puzzling mainly          */

#if !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__)
#define _FASTCALL __fastcall
#define _STDCALL  __stdcall
#else
#define _FASTCALL
#define _STDCALL
#endif /* !defined(_UNKNOWN_ENVIRONMENT_) && !defined(__GNUC__) */

template <class T>
inline T CLAMP(const T& X,const T& LoX,const T& HiX) { return (X >= LoX)?((X <= HiX)?(X):(HiX)):(LoX); }

/* PPMd module works with file streams via ...GETC/...PUTC macros only      */
typedef FILE _PPMD_FILE;
#define _PPMD_E_GETC(fp)   getc(fp)
#define _PPMD_E_PUTC(c,fp) putc((c),fp)
#define _PPMD_D_GETC(fp)   getc(fp)
#define _PPMD_D_PUTC(c,fp) putc((c),fp)
/******************  Example of C++ buffered stream  ************************
class PRIME_STREAM {
public:
enum { BUF_SIZE=64*1024 };
    PRIME_STREAM(): Error(0), StrPos(0), Count(0), p(Buf) {}
    int  get(     ) { return (--Count >= 0)?(*p++    ):( fill( )); }
    int  put(int c) { return (--Count >= 0)?(*p++ = c):(flush(c)); }
    int  getErr() const { return Error; }
    int    tell() const { return StrPos+(p-Buf); }
    BOOL  atEOS() const { return (Count < 0); }
protected:
    int Error, StrPos, Count;
    BYTE* p, Buf[BUF_SIZE];
    virtual int  fill(     ) = 0;           // it must fill Buf[]
    virtual int flush(int c) = 0;           // it must remove (p-Buf) bytes
};
typedef PRIME_STREAM _PPMD_FILE;
#define _PPMD_E_GETC(pps)   (pps)->get()
#define _PPMD_E_PUTC(c,pps) (pps)->put(c)
#define _PPMD_D_GETC(pps)   (pps)->get()
#define _PPMD_D_PUTC(c,pps) (pps)->put(c)
**************************  End of example  *********************************/

#endif /* !defined(_PPMDTYPE_H_) */
