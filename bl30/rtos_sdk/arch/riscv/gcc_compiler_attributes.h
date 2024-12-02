/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __GCC_COMPILER_ATTRIBUTES_H__
#define __GCC_COMPILER_ATTRIBUTES_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __weak
#define __weak __attribute__((__weak__))
#endif

#ifndef __packed
#define __packed __attribute__((__packed__))
#endif

#ifndef __has_builtin
#define __has_builtin(x) (0)
#endif

#ifndef __noinline
#define __noinline __attribute__((noinline))
#endif

#ifndef __inline
#define __inline inline
#endif

#ifndef __static_inline
#define __static_inline static inline
#endif

#ifndef __static_forceinline
#define __static_forceinline __attribute__((always_inline)) static inline
#endif

#ifndef __no_return
#define __no_return __attribute__((__noreturn__))
#endif

#ifndef __used
#define __used __attribute__((used))
#endif

#ifndef __vector_size
#define __vector_size(x) __attribute__((vector_size(x)))
#endif

#ifndef __packed_struct
#define __packed_struct struct __attribute__((packed, aligned(1)))
#endif

#ifndef __packed_union
#define __packed_union union __attribute__((packed, aligned(1)))
#endif

#ifndef __aligned
#define __aligned(x) __attribute__((aligned(x)))
#endif

#ifndef __restrict
#define __restrict __restrict
#endif

#ifndef __compiler_barrier
#define __compiler_barrier() __asm volatile("":::"memory")
#endif

#ifndef __usually
#define __usually(exp) __builtin_expect((exp), 1)
#endif

#ifndef __rarely
#define __rarely(exp) __builtin_expect((exp), 0)
#endif

#ifndef __interrupt
#define __interrupt __attribute__((interrupt))
#endif

#ifndef __machine_interrupt
#define __machine_interrupt __attribute__((interrupt ("machine")))
#endif

#ifndef __supvisor_interrupt
#define __supvisor_interrupt __attribute__((interrupt ("supervisor")))
#endif

#ifndef __user_interrupt
#define __user_interrupt __attribute__((interrupt ("user")))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __GCC_COMPILER_ATTRIBUTES_H__ */
