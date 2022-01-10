/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __XLAT_TABLES_PRIVATE_H__
#define __XLAT_TABLES_PRIVATE_H__

#define LOG_LEVEL 0
#define LOG_LEVEL_VERBOSE		50
#define DEBUG 0
#define ADDR_SPACE_SIZE			(1ull << 32)

#define IS_POWER_OF_TWO(x)			\
	(((x) & ((x) - 1)) == 0)

#define CASSERT(cond, msg)	\
	typedef char msg[(cond) ? 1 : -1] __unused

/* The virtual address space size must be a power of two. */
CASSERT(IS_POWER_OF_TWO(ADDR_SPACE_SIZE), assert_valid_addr_space_size);

void init_xlation_table(uintptr_t base_va, uint64_t *table,
			int level, uintptr_t *max_va,
			unsigned long long *max_pa);

#endif /* __XLAT_TABLES_PRIVATE_H__ */
