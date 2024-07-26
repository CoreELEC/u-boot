/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

#include "lscript.h"

OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv",
              "elf32-littleriscv")
OUTPUT_ARCH("riscv")
ENTRY( _start )
MEMORY
{
    ram (rw) : ORIGIN = configMEM_START, LENGTH = configMEM_LEN
}
SECTIONS
{
    __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;
    .text :
    {
        . = ALIGN(4);
 PROVIDE( _text = . );
 *(.init)
        *(.text* .rodata*)
        . = ALIGN(0x400);
        PROVIDE( _etext = . );
 *(.vtable)
    } > ram
    .data :
    {
	. = ALIGN(4);
        _data = .;
        *(vtable)
	. = ALIGN(4);
	_module_pm_begin = .;
	KEEP (*(SORT(.sdata.__initcall*)))
	_module_pm_end = .;
        *(.sdata* .data*)
	. = ALIGN(4);
        _edata = .;
    } > ram
    .bss :
    {
        _bss = .;
        *(.sbss*)
        *(.gnu.linkonce.sb.*)
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        . = ALIGN(4);
        _ebss = .;
    } > ram
    .stack :
    {
        PROVIDE( _stack = . );
 . = ALIGN(1024);
        . += __stack_size;
        PROVIDE( _sp = . );
    } >ram
}
