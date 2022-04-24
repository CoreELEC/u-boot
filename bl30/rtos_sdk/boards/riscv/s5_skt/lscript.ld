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
 KEEP (*(SORT_NONE(.init)))
        . = ALIGN(4);
        *(.text)
        *(.rodata*)
        _etext = .;
    } > ram
    .data :
    {
        _data = .;
        *(vtable)
        *(.data)
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

 . = ALIGN(1024);
        . += __stack_size;
        _sp = .;
    } >ram
    .init :
    {
     KEEP (*(SORT_NONE(.init)))
    } >ram
}
