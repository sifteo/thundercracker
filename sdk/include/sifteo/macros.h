/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>

/**
 * @defgroup macros Macros
 *
 * @brief Utility, logging, and scripting macros
 *
 * This section includes common C++ utility macros, as well as macros which
 * implement platform-specific logging and debugging support.
 *
 * @{
 */

/** 
 * @brief Disable automatic inlining for one function
 *
 * For example:
 *
 *     void NOINLINE myFunction() {
 *         // Won't be inlined
 *     }
 *
 * This attribute may be useful in cases where you want to explicitly prevent
 * a function's code from being included into its callers, for debugging or
 * for cache optimization reasons.
 *
 * @hideinitializer
 */
#define NOINLINE        __attribute__ ((noinline))

/** 
 * @brief Always enable inlining for one function
 *
 * For example:
 *
 *     void ALWAYS_INLINE myFunction() {
 *         // Must be inlined
 *     }
 *
 * This attribute may be useful in cases where a function would normally
 * not be inlined, but for either performance or correctness reasons you
 * need it to be. Inlined functions are just as fast as macros.
 *
 * @hideinitializer
 */
#define ALWAYS_INLINE   __attribute__ ((always_inline))

/// A macro wrapper for the C preprocessor stringification operator.
/// @hideinitializer
#define STRINGIFY(_x)   #_x

/**
 * @brief A second-level wrapper around the C preprocessor stringification operator.
 *
 * Necessary if you wish to stringify tokens after macro expansion
 * rather than before.
 *
 * @hideinitializer
 */
#define TOSTRING(_x)    STRINGIFY(_x)

/// Expands to a string literal uniquely identifying the file and source line where it appears.
/// @hideinitializer
#define SRCLINE         __FILE__ ":" TOSTRING(__LINE__)

/**
 * @brief Mark a chunk of code as debug-only
 *
 * This code will be disabled at link-time on release builds. For example:
 *
 *     DEBUG_ONLY(doDebugStuff());
 *
 *     DEBUG_ONLY({
 *         // Do some inlined debug stuff here.
 *     });
 *
 * This macro acts like a C statement, and it may include any C statement
 * or block. The supplied code is _not_ removed at compile-time, so any
 * compile-time side effects (such as using the value of a variable)
 * always occur. The argument is wrapped in a _SYS_lti_isDebug() test.
 * This function evaluates to either true or false at link-time. So,
 * at link-time, if the `-g` flag was not passed to Slinky, the enclosed
 * code is optimized out.
 *
 * @hideinitializer
 */
#define DEBUG_ONLY(_x) \
do { \
    if (_SYS_lti_isDebug()) { \
        _x \
    } \
} while (0)

#ifdef NO_LOG
#   define LOG(...)
#else
    /**
     * @brief Smart printf-like logging macro
     *
     * This macro acts like printf(), but with some important differences:
     * - Most but not all standard format specifiers are supported.
     * - Many new format specifiers are available, as listed below.
     * - Like the DEBUG_ONLY macro, these macros always generate code at
     *   compile-time, but they are fully optimized out at link-time when
     *   linking a release binary.
     * - Any parameters which are known to be constant at link-time are
     *   expanded right away, and no corresponding code is generated. This
     *   also means that string literals inserted with `%%s` behave exactly
     *   the same as if the corresponding string had been escaped properly
     *   and included as part of the format string.
     *
     * In fact, the linker does a lot of work to support these logging macros
     * in the most efficient way possible. The format string is parsed by
     * Slinky, and a sequence of logging _packets_ are generated. These log
     * packets may contain raw binary data (for strings, hex dumps) or they
     * may contain a set of unformatted printf()-like parameters, along with
     * a reference to a format string in an external string table.
     *
     * The format string is _not stored_ in your game's read-only data segment.
     * Instead, it is part of your game's debug symbols. This means that when
     * running a debug build on real hardware, the format strings never have
     * to be installed on the Base. Instead, your PC uses the binary's debug
     * symbols to decode very terse un-formatted log packets back into the
     * full text output.
     *
     * Normally you would disable debugging by linking your binary as a release
     * build. However, sometimes it can be useful to have a build with symbols
     * but without LOGs. For example, this can be used to isolate problems that,
     * for whatever reason, only show up with LOGs disabled. You can do this
     * by setting the -DNO_LOG compiler option.
     *
     * Logging supports a variety of format specifiers. Most of the standard
     * printf() specifiers are supported, plus we have several new ones:
     * - Literal characters, and `%%`
     * - Standard integer specifiers: `%%d, %%i, %%o, %%u, %%X, %%x, %%p, %%c`
     * - Standard float specifiers: `%%f, %%F, %%e, %%E, %%g, %%G`
     * - Four chars packed into a 32-bit integer: `%%C`
     * - Binary integers: `%%b`
     * - C-style strings: `%%s`
     * - Hex-dump of fixed width buffers: `%(width in bytes)h`
     * - Pointer, printed as a resolved symbol when possible: `%%P`
     *
     * @hideinitializer
     */
#   define LOG(...) \
do { \
    if (_SYS_lti_isDebug()) \
        _SYS_lti_log(__VA_ARGS__); \
} while (0)
#endif

#ifdef NO_ASSERT
#   define ASSERT(_x)
#else
    /**
     * @brief Runtime debug assertion
     *
     * On release builds, ASSERT has no effect. On debug builds, the argument
     * is evaluated, and if it is False, the current program aborts with an
     * Abort fault, after logging the text of the failed test, along with
     * the line number and file where it occurred.
     *
     * Normally you would disable ASSERT by linking your binary as a release
     * build. However, sometimes it can be useful to have a build with symbols
     * but without assertions. For example, this can be used to isolate problems that,
     * for whatever reason, only show up with assertions disabled. You can do this
     * by setting the -DNO_ASSERT compiler option.
     *
     * @hideinitializer
     */
#   define ASSERT(_x) \
do { \
    if (_SYS_lti_isDebug() && !(_x)) { \
        _SYS_lti_log("ASSERT failure at %s:%d, (%s)\n", __FILE__, __LINE__, #_x); \
        _SYS_abort(); \
    } \
} while (0)
#endif

/**
 * This macro is used as part of the implementation of SCRIPT() and SCRIPT_FMT(),
 * but it may also be useful on its own in special circumstances.
 *
 * This macro sets the current scripting backend which receives log messages.
 * If the scripting backend is set to `NONE`, LOG() messages behave normally.
 * If a different scripting engine is specified, however, fully formatted
 * LOG() text is accumulated in a buffer in Siftulator until such time as
 * the scripting backend is set back to `NONE`. At that moment, the scripting
 * backend has a chance to synchronously interpret the buffer of logged text.
 *
 * @hideinitializer
 */
#define SCRIPT_TYPE(_type) \
do { \
    _SYS_log((_SYS_SCRIPT_ ## _type) | (_SYS_LOGTYPE_SCRIPT << 27), \
        0,0,0,0,0,0,0); \
} while (0)

/**
 * @brief Inline emulator scripting, for automated testing and more.
 *
 * Scripting is a feature available only when running debug builds in
 * Siftulator. All inline scripting is removed at link-time when building
 * a Release binary.
 *
 * This macro runs the specified 'code' in Siftulator's scripting environment.
 * 'type' indicates which scripting language/environment the code is intended
 * for. Right now the only supported type is `LUA`.
 *
 * For example:
 *
 *     SCRIPT(LUA,
 *         package.path = package.path .. ";scripts/?.lua"
 *         require('my-test-library')
 *     );
 *
 *     SCRIPT(LUA, invokeTest());
 *
 *     SCRIPT(LUA, System():setAssetLoaderBypass(true));
 *
 *     SCRIPT(LUA, Cube(0):saveScreenshot("myScreenshot.png"));
 *
 *     SCRIPT(LUA, Cube(0):testScreenshot("myScreenshot.png"));
 *
 * These script fragments are all executed in the same global Lua context.
 * Each script fragment executes synchronously with your game code.
 *
 * See the @ref scripting documentation for more info.
 *
 * @hideinitializer
 */
#define SCRIPT(_type, _code) \
do { \
    if (_SYS_lti_isDebug()) { \
        SCRIPT_TYPE(_type); \
        _SYS_lti_log("%s", #_code); \
        SCRIPT_TYPE(NONE); \
    } \
} while (0)

/**
 * @brief Like SCRIPT(), but this variant supports format specifiers
 *
 * This can be used to pass parameters back and forth
 * between Lua code and your game's code, when running in Siftulator.
 *
 * For example:
 * 
 *     SCRIPT_FMT(LUA, "Frontend():postMessage('Power is >= %d')", 9000);
 *
 *     int luaGetInteger(const char *expr)
 *     {
 *         int result;
 *         SCRIPT_FMT(LUA, "Runtime():poke(%p, %s)", &result, expr);
 *         return result;
 *     }
 *
 *     void luaSetInteger(const char *varName, int value)
 *     {
 *         SCRIPT_FMT(LUA, "%s = %d", varName, value);
 *     }
 *
 * This is implemented using the same formatter as LOG(), so all of the
 * format specifiers and features described in LOG()'s documentation apply
 * to SCRIPT_FMT() as well.
 *
 * @hideinitializer
 */
#define SCRIPT_FMT(_type, ...) \
do { \
    if (_SYS_lti_isDebug()) { \
        SCRIPT_TYPE(_type); \
        _SYS_lti_log(__VA_ARGS__); \
        SCRIPT_TYPE(NONE); \
    } \
} while (0)

/// Convenience macro for tracing the name and value of an integer expression
/// @hideinitializer
#define LOG_INT(_x)     LOG("%s = %d\n", #_x, (_x));

/// Convenience macro for tracing the name and value of an expression, in hexadecimal
/// @hideinitializer
#define LOG_HEX(_x)     LOG("%s = 0x%08x\n", #_x, (_x));

/// Convenience macro for tracing the name and value of a floating point expression
/// @hideinitializer
#define LOG_FLOAT(_x)   LOG("%s = %f\n", #_x, (_x));

/// Convenience macro for tracing the name and value of a C-style string
/// @hideinitializer
#define LOG_STR(_x)     LOG("%s = \"%s\"\n", #_x, (const char*)(_x));

/// Convenience macro for tracing the name and value of an integer 2-vector expression
/// @hideinitializer
#define LOG_INT2(_x)    LOG("%s = (%d, %d)\n", #_x, (_x).x, (_x).y);

/// Convenience macro for tracing the name and value of an integer 3-vector expression
/// @hideinitializer
#define LOG_INT3(_x)    LOG("%s = (%d, %d, %d)\n", #_x, (_x).x, (_x).y, (_x).z);

/// Convenience macro for tracing the name and value of a floating point 2-vector expression
/// @hideinitializer
#define LOG_FLOAT2(_x)  LOG("%s = (%f, %f)\n", #_x, (_x).x, (_x).y);

/// Produces a 'size of array is negative' compile error when the assert fails
/// @hideinitializer
#define STATIC_ASSERT(_x)  ((void)sizeof(char[1 - 2*!(_x)]))

#ifndef MIN
/// Macro which returns the smallest of two values. Values are evaluated twice.
/// @hideinitializer
#define MIN(a,b)   ((a) < (b) ? (a) : (b))
/// Macro which returns the largest of two values. Values are evaluated twice.
/// @hideinitializer
#define MAX(a,b)   ((a) > (b) ? (a) : (b))
#endif

#ifndef NULL
/// Invalid pointer. Equal to zero.
/// @hideinitializer
#define NULL 0
#endif

#ifndef arraysize
/// Calculate the number of elements in a C++ array, using sizeof.
/// @hideinitializer
#define arraysize(a)   (sizeof(a) / sizeof((a)[0]))
#endif

#ifndef offsetof
/// Calculate the byte offset from the beginning of structure `t` to member `m`
/// @hideinitializer
#define offsetof(t,m)  ((uintptr_t)(uint8_t*)&(((t*)0)->m))
#endif

/**
 * @} endgroup macros
*/
