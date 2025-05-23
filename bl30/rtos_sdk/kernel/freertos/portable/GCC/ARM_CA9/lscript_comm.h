#ifndef _LSCRIPT_COMM_H_
#define _LSCRIPT_COMM_H_

#ifndef __used
#if GCC_VERSION < 30300
# define __used			__attribute__((__unused__))
#else
# define __used			__attribute__((__used__))
#endif
#endif

#ifdef CONFIG_LTO_CLANG
  /* prepend the variable name with __COUNTER__ to ensure correct ordering */
  #define ___initcall_name2(c, fn, id) 	__initcall_##c##_##fn##id
  #define ___initcall_name1(c, fn, id)	___initcall_name2(c, fn, id)
  #define __initcall_name(fn, id) 	___initcall_name1(__COUNTER__, fn, id)
#else
  #define __initcall_name(fn, id) 	__initcall_##fn##id
#endif

#define __define_initcall(fn, id) \
	static int (*__initcall_name(fn, id))(void) __used \
	__attribute__((__section__(".initcall" #id ".init"))) = (int (*)(void))fn;

#define early_initcall(fn)		__define_initcall(fn, early)
#define pure_initcall(fn)		__define_initcall(fn, 0)
#define core_initcall(fn)		__define_initcall(fn, 1)

#define INIT_CALLS_LEVEL(level)						\
		KEEP(*(.initcall##level.init))			\
		KEEP(*(.initcall##level##s.init))

#define INIT_CALLS							\
		KEEP(*(.initcallearly.init))		\
		INIT_CALLS_LEVEL(0)					\
		INIT_CALLS_LEVEL(1)					\
		INIT_CALLS_LEVEL(2)					\
		INIT_CALLS_LEVEL(3)					\
		INIT_CALLS_LEVEL(4)					\
		INIT_CALLS_LEVEL(5)					\
		INIT_CALLS_LEVEL(6)					\
		INIT_CALLS_LEVEL(7)					\
		KEEP (*(.preinit_array))				\
		KEEP (*(.init_array*))

#endif
