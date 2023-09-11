/*
 * Copyright (c) 2021-2022 Amlogic, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: MIT
 */

OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv",
              "elf32-littleriscv")
OUTPUT_ARCH("riscv")
ENTRY( _start )
MEMORY
{
    ram (rw) : ORIGIN = 0x10000000, LENGTH = (64 * 1024)
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
        . = ALIGN(0x100);
        PROVIDE( _etext = . );
 *(.vtable)
    } > ram
    .data :
    {
	. = ALIGN(4);
        _data = .;
        *(vtable)
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
