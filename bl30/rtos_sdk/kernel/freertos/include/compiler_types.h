#ifndef __COMPILER_TYPES_H_
#define __COMPILER_TYPES_H_
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLY__
/* Compiler specific macros. */
#ifdef __clang__
#include <compiler-clang.h>
#elif defined(__INTEL_COMPILER)
#include <compiler-intel.h>
#elif defined(__GNUC__)
/* The above compilers also define __GNUC__, so order is important here. */
#include <compiler-gcc.h>
#else
#error "Unknown compiler"
#endif

#define __aligned_largest	__attribute__((aligned))
#define __printf(a, b)		__attribute__((format(printf, a, b)))
#define __scanf(a, b)		__attribute__((format(scanf, a, b)))
#define __maybe_unused		__attribute__((unused))
#define __always_unused		__attribute__((unused))
#define __mode(x)		__attribute__((mode(x)))
#define __malloc		__attribute__((__malloc__))
#define __noreturn		__attribute__((noreturn))
#define __weak			__attribute__((weak))
#define __alias(symbol)		__attribute__((alias(#symbol)))
#define __cold			__attribute__((cold))

#ifndef __always_inline
#define __always_inline inline __attribute__((always_inline))
#endif

#endif

#ifdef __cplusplus
}
#endif
#endif
