/******************************************************************************
 *
 * standalone.ld - Linker script for applications using startup.c and
 *                 DriverLib.
 *
 * Copyright (c) 2005-2007 Luminary Micro, Inc.  All rights reserved.
 *
 * Software License Agreement
 *
 * Luminary Micro, Inc. (LMI) is supplying this software for use solely and
 * exclusively on LMI's microcontroller products.
 *
 * The software is owned by LMI and/or its suppliers, and is protected under
 * applicable copyright laws.  All rights are reserved.  Any use in violation
 * of the foregoing restrictions may subject the user to criminal sanctions
 * under applicable laws, as well as to civil liability for the breach of the
 * terms and conditions of this license.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS".  NO WARRANTIES, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE.
 * LMI SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 * This is part of revision 1049 of the Stellaris Driver Library.
 *
 *****************************************************************************/

OUTPUT_FORMAT("elf32-littleriscv", "elf32-littleriscv",
              "elf32-littleriscv")
OUTPUT_ARCH("riscv")
ENTRY( _start )


MEMORY
{
/*    ram (rw) : ORIGIN = 0x10000000 + 16*1024, LENGTH = (80 * 1024)*/
    ram (rw) : ORIGIN = 0xf7028000, LENGTH = (80 * 1024)
}

SECTIONS
{
    __stack_size = DEFINED(__stack_size) ? __stack_size : 2K;

    .text :
    {
	*(.vtable)
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

/*    .stack ORIGIN(ram) + LENGTH(ram) - __stack_size - 45*1204:*/
    .stack :
    {
        PROVIDE( _heap_end = . );
	. = ALIGN(1024);
        . += __stack_size;
        PROVIDE( _sp = . );
    } >ram

    .init :
    {
        KEEP (*(SORT_NONE(.init)))
    } >ram
}
