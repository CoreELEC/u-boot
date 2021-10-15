#ifndef __COMPILER_H_
#error "Please don't include <compiler-gcc.h> directly, include <compiler.h> instead."
#endif

/*
 * Common definitions for all gcc versions go here.
 */
#define GCC_VERSION (__GNUC__ * 10000		\
		     + __GNUC_MINOR__ * 100	\
		     + __GNUC_PATCHLEVEL__)

#if GCC_VERSION >= 70000
#define KASAN_ABI_VERSION 5
#elif GCC_VERSION >= 50000
#define KASAN_ABI_VERSION 4
#elif GCC_VERSION >= 40902
#define KASAN_ABI_VERSION 3
#endif

#if GCC_VERSION >= 40902
/*
 * Tell the compiler that address safety instrumentation (KASAN)
 * should not be applied to that function.
 * Conflicts with inlining: https://gcc.gnu.org/bugzilla/show_bug.cgi?id=67368
 */
#define __no_sanitize_address __attribute__((no_sanitize_address))
#endif

#if !defined(__no_sanitize_address)
#define __no_sanitize_address
#endif
