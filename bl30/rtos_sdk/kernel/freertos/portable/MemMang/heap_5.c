/*
 * FreeRTOS Kernel V10.2.1
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/*
 * A sample implementation of pvPortMalloc() that allows the heap to be defined
 * across multiple non-contigous blocks and combines (coalescences) adjacent
 * memory blocks as they are freed.
 *
 * See heap_1.c, heap_2.c, heap_3.c and heap_4.c for alternative
 * implementations, and the memory management pages of http://www.FreeRTOS.org
 * for more information.
 *
 * Usage notes:
 *
 * vPortDefineHeapRegions() ***must*** be called before pvPortMalloc().
 * pvPortMalloc() will be called if any task objects (tasks, queues, event
 * groups, etc.) are created, therefore vPortDefineHeapRegions() ***must*** be
 * called before any other objects are defined.
 *
 * vPortDefineHeapRegions() takes a single parameter.  The parameter is an array
 * of HeapRegion_t structures.  HeapRegion_t is defined in portable.h as
 *
 * typedef struct HeapRegion
 * {
 *	uint8_t *pucStartAddress; << Start address of a block of memory that will be part of the heap.
 *	size_t xSizeInBytes;	  << Size of the block of memory.
 * } HeapRegion_t;
 *
 * The array is terminated using a NULL zero sized region definition, and the
 * memory regions defined in the array ***must*** appear in address order from
 * low address to high address.  So the following is a valid example of how
 * to use the function.
 *
 * HeapRegion_t xHeapRegions[] =
 * {
 * 	{ ( uint8_t * ) 0x80000000UL, 0x10000 }, << Defines a block of 0x10000 bytes starting at address 0x80000000
 * 	{ ( uint8_t * ) 0x90000000UL, 0xa0000 }, << Defines a block of 0xa0000 bytes starting at address of 0x90000000
 * 	{ NULL, 0 }                << Terminates the array.
 * };
 *
 * vPortDefineHeapRegions( xHeapRegions ); << Pass the array into vPortDefineHeapRegions().
 *
 * Note 0x80000000 is the lower address so appears in the array first.
 *
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Defining MPU_WRAPPERS_INCLUDED_FROM_API_FILE prevents task.h from redefining
all the API functions to use the MPU wrappers.  That should only be done when
task.h is included from an application file. */
#define MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#include "FreeRTOS.h"
#include "task.h"
#ifdef ARCH64
#include "sys_printf.h"
#include "stacktrace_64.h"
#endif
#if ENABLE_KASAN
#include <kasan/kasan.h>
#endif

#undef MPU_WRAPPERS_INCLUDED_FROM_API_FILE

#if( configSUPPORT_DYNAMIC_ALLOCATION == 0 )
	#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

/* Block sizes must not get too small. */
#define heapMINIMUM_BLOCK_SIZE	( ( size_t ) ( xHeapStructSize << 1 ) )

/* Assumes 8bit bytes! */
#define heapBITS_PER_BYTE		( ( size_t ) 8 )

/* Allocate the memory for the heap. */
#if N200_REVA
#define MAX_REGION_CNT 2
static HeapRegion_t xDefRegion[MAX_REGION_CNT+1]=
{
	{(uint8_t *)configDEFAULT_HEAP_ADDR, configDEFAULT_HEAP_SIZE},
	{0, 0},
	{0, 0}
};
#else
extern uint8_t _heap_start[];
extern uint8_t _heap_len[];
#define MAX_REGION_CNT 2
static HeapRegion_t xDefRegion[MAX_REGION_CNT+1]=
{
	{_heap_start, (size_t)_heap_len},
	{0, 0},
	{0, 0}
};
#endif

/* Define the linked list structure.  This is used to link free blocks in order
of their memory address. */
typedef struct A_BLOCK_LINK
{
	struct A_BLOCK_LINK *pxNextFreeBlock;	/*<< The next free block in the list. */
	size_t xBlockSize;						/*<< The size of the free block. */
} BlockLink_t;

/*-----------------------------------------------------------*/

/*
 * Inserts a block of memory that is being freed into the correct position in
 * the list of free memory blocks.  The block being freed will be merged with
 * the block in front it and/or the block behind it if the memory blocks are
 * adjacent to each other.
 */
static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert );

/*-----------------------------------------------------------*/

/* The size of the structure placed at the beginning of each allocated memory
block must by correctly byte aligned. */
static const size_t xHeapStructSize	= ( sizeof( BlockLink_t ) + ( ( size_t ) ( portBYTE_ALIGNMENT - 1 ) ) ) & ~( ( size_t ) portBYTE_ALIGNMENT_MASK );

/* Create a couple of list links to mark the start and end of the list. */
static BlockLink_t xStart, *pxEnd = NULL;

/* Keeps track of the number of free bytes remaining, but says nothing about
fragmentation. */
static size_t xFreeBytesRemaining = 0U;
static size_t xMinimumEverFreeBytesRemaining = 0U;
static size_t xTotalHeapBytes = 0U;

/* Gets set to the top bit of an size_t type.  When this bit in the xBlockSize
member of an BlockLink_t structure is set then the block belongs to the
application.  When the bit is free the block is still part of the free heap
space. */
static size_t xBlockAllocatedBit = 0;

/*-----------------------------------------------------------*/
#ifdef CONFIG_MEMORY_LEAK
struct MemLeak MemLeak_t[CONFIG_MEMLEAK_ARRAY_SIZE];
#endif

#if CONFIG_MEMORY_LEAK2 && CONFIG_STACK_TRACE
typedef uint32_t addr_t;
typedef uint16_t id_t;

typedef union
{
    uint32_t data;
    struct {
        uint16_t hi_data;
        uint16_t lo_data;
    };
}MLeakEntry_t;
#define MLeakEntry_SIZE   sizeof(MLeakEntry_t)
#define MLeakEntry_ID_AUX    0
#define MLeakEntry_ID_LEN    1
#define MLeakEntry_ID_ADDR   2
#define MLeakEntry_ID_TRACE  3
#define MLeakEntry_TRACE_CNT  5
#define MLeakEntry_TRACE_LEN  (MLeakEntry_TRACE_CNT*MLeakEntry_SIZE)
#define MLeakEntry_CNT    8
#define MLeak_MAX_RCD     (128*1024/(MLeakEntry_CNT*MLeakEntry_SIZE))
static MLeakEntry_t *mleak_data;
static id_t *mleak_data_order;
static uint32_t mleak_data_cnt;
static id_t mleak_data_hdr;
#define MLeak_DATA(i) mleak_data[i]
#define MLeakEntry_AUX_HI(i) MLeak_DATA((i)*(MLeakEntry_CNT)+MLeakEntry_ID_AUX).hi_data
#define MLeakEntry_AUX_LO(i) MLeak_DATA((i)*(MLeakEntry_CNT)+MLeakEntry_ID_AUX).lo_data
#define MLeakEntry_LEN(i) MLeak_DATA((i)*(MLeakEntry_CNT)+MLeakEntry_ID_LEN).data
#define MLeakEntry_ADDR(i) MLeak_DATA((i)*(MLeakEntry_CNT)+MLeakEntry_ID_ADDR).data
#define MLeakEntry_TRACE(i,j) MLeak_DATA((i)*(MLeakEntry_CNT)+MLeakEntry_ID_TRACE+j).data

static uint32_t mleak_enable=0;
static uint64_t malloc_cnt=0,free_cnt=0;
extern void on_malloc(void *p, int32_t len);
extern int32_t get_calltrace(addr_t *trace, int32_t num);
int32_t get_calltrace(addr_t *trace, int32_t num)
{
#define CT_SKIP 3
	unsigned long _trace[32];
	int32_t ret,i;
	if ( num > ( 32 - CT_SKIP ) ) num = ( 32 - CT_SKIP );
	ret=get_backtrace(NULL, _trace, CT_SKIP+num);
	if ( ret <= CT_SKIP )return 0;
	for ( i = 0; i < ( ret - CT_SKIP ); i++) {
		trace[i]=_trace[i+CT_SKIP];
	}
	return ret-CT_SKIP;
}
static void print_traceitem(addr_t addr)
{
	print_symbol(addr);
	return;
}
static id_t _find_addr(addr_t addr, id_t *pptr)
{
	uint32_t start=1,end=mleak_data_cnt-1,i;
	addr_t tmpval=0;
	while ( start <= end ) {
		i=(start+end)/2;
		tmpval=MLeakEntry_ADDR(mleak_data_order[i]);
		configASSERT(tmpval);
		if ( addr == tmpval ) {
			if (pptr) *pptr = i;
			return mleak_data_order[i];
		}
		if (addr > tmpval) start=i+1;
		else end=i-1;
	}
	if (pptr)*pptr=end;
	return 0;
}
void on_malloc(void *p, int32_t len)
{
	addr_t addr=(addr_t)(uint64_t)p;
	id_t i,j,m;
	if (!p)return;
	if (mleak_enable != 1)
		return;
	malloc_cnt++;
	if (mleak_data_cnt >= MLeak_MAX_RCD)
		return ;
	i=_find_addr(addr,&j);
	if (i)return;
	configASSERT(mleak_data_hdr&& mleak_data_hdr<MLeak_MAX_RCD);
	i=mleak_data_hdr;
	mleak_data_hdr=MLeakEntry_AUX_LO(i);
	MLeakEntry_LEN(i)=len;
	MLeakEntry_ADDR(i)=addr;
	memset(&MLeakEntry_TRACE(i,0),0,MLeakEntry_TRACE_LEN);
	get_calltrace(&MLeakEntry_TRACE(i,0),MLeakEntry_TRACE_CNT);

	for (m=mleak_data_cnt;m>(j+1);m--)
		mleak_data_order[m]=mleak_data_order[m-1];
	mleak_data_order[j+1]=i;
	mleak_data_cnt++;
}
static void on_free(void *p)
{
	addr_t addr=(addr_t)(uint64_t)p;
	id_t i,j;
	if (!p)return;
	if (mleak_enable != 1)
		return;
	free_cnt++;
	if (mleak_data_cnt <= 1)
		return;
	i=_find_addr(addr,&j);
	if (i == 0)return;
	MLeakEntry_ADDR(i)=0;
	MLeakEntry_AUX_LO(i)=(id_t)mleak_data_hdr;
	mleak_data_hdr=i;

	for (;j<(mleak_data_cnt-1);j++)
		mleak_data_order[j]=mleak_data_order[j+1];
	mleak_data_cnt--;
}
static void mleak_reset(void)
{
	uint32_t i;
	malloc_cnt=0;
	free_cnt=0;
	mleak_data_cnt=1;
	mleak_data_hdr=1;
	memset(mleak_data,0,MLeak_MAX_RCD*MLeakEntry_CNT*MLeakEntry_SIZE);
	memset(mleak_data_order,0,MLeak_MAX_RCD*sizeof(mleak_data_order[0]));
	for (i=1;i<(MLeak_MAX_RCD-1);i++) {
		MLeakEntry_AUX_LO(i)=i+1;
	}
	MLeakEntry_AUX_LO(MLeak_MAX_RCD-1)=0;
}

static uint32_t mleak_memcost(void)
{
	return (MLeak_MAX_RCD*MLeakEntry_CNT*MLeakEntry_SIZE+MLeak_MAX_RCD*sizeof(mleak_data_order[0]));
}
static int32_t _mleak_cmp_trace(id_t i1, id_t i2)
{
	int32_t i;
	for (i = 0;i < MLeakEntry_TRACE_CNT && MLeakEntry_TRACE(i1,i); i++) {
		if (MLeakEntry_TRACE(i1,i) != MLeakEntry_TRACE(i2,i))
			break;
	}
	if (i >= MLeakEntry_TRACE_CNT)
		return 0;
	if (MLeakEntry_TRACE(i1,i) == MLeakEntry_TRACE(i2,i))
		return 0;
	else if (MLeakEntry_TRACE(i1,i) > MLeakEntry_TRACE(i2,i))
		return 1;
	else
		return -1;
}
static void mleak_show_result(void)
{
	uint32_t i,j,m;
	uint32_t size,idx=0;
	printf("malloc_cnt %lu free_cnt %lu,toolcost=%u\n",malloc_cnt,free_cnt,mleak_memcost());
	for (i=1;i<MLeak_MAX_RCD;i++) {
		if (MLeakEntry_ADDR(i))MLeakEntry_AUX_HI(i)=-1;
		else MLeakEntry_AUX_HI(i)=0;
	}
	for (i=1;i<MLeak_MAX_RCD;i++) {
		if (MLeakEntry_AUX_HI(i) == (uint16_t)-1) {
			MLeakEntry_AUX_HI(i)=0;
			printf("CallTrace[%u]:\n",idx++);
			for (m=0;m<MLeakEntry_TRACE_CNT;m++) {
				if (MLeakEntry_TRACE(i,m))print_traceitem(MLeakEntry_TRACE(i,m));
			}
			m=0;
			size=0;
			printf("[%u] [%08x,%08x]\n",m++, MLeakEntry_ADDR(i),MLeakEntry_LEN(i));
			size+=MLeakEntry_LEN(i);
			for (j=i+1;j<MLeak_MAX_RCD;j++) {
				if (MLeakEntry_AUX_HI(j) == (uint16_t)-1&&
					_mleak_cmp_trace(i,j)==0) {
					MLeakEntry_AUX_HI(j)=0;
					printf("[%u] [%08x,%08x]\n",m++, MLeakEntry_ADDR(j),MLeakEntry_LEN(j));
					size+=MLeakEntry_LEN(j);
				}
			}
			printf("buffer_count %u total_size %u\n",m,size);
		}
	}
}

int32_t mleak_init(void)
{
	static int inited=0;
	if (!inited) {
		mleak_data=malloc(MLeak_MAX_RCD*MLeakEntry_CNT*MLeakEntry_SIZE);
		if (!mleak_data)
			goto _exit_fun;
		mleak_data_order=malloc(MLeak_MAX_RCD*sizeof(mleak_data_order[0]));
		if (!mleak_data_order)
			goto free_mleak_data;
		inited = 1;
	}
	mleak_reset();
	return 0;
free_mleak_data:
	free(mleak_data);
_exit_fun:
	printf("Error: memleak tool init fail!\n");
	return -1;
}
void mleak_set_enable(int val)
{
#ifdef ARCH64
	unsigned long flags;
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif
	if (val) {
		if (mleak_enable == 0) {
			if (!mleak_init())
				mleak_enable=1;
		}
	}else{
		if (mleak_enable == 1) {
			mleak_enable=0;
			mleak_show_result();
		}
	}
#ifdef ARCH64
	portIRQ_RESTORE(flags);
#else
	( void ) xTaskResumeAll();
#endif
}
static BaseType_t prvMemleak2Command( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString );
static const CLI_Command_Definition_t xMemLeak2Command =
{
	"memleak2",
	"\r\nmemleak2: \r\n Enable/Disable memleak check: memleak2 on/off\r\n",
	prvMemleak2Command,
	1
};
static BaseType_t prvMemleak2Command( char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString )
{
	BaseType_t xLengthParameter = 0;
	const char *param;

	param = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xLengthParameter);
	if (param == NULL) {
		memcpy( pcWriteBuffer, "Not find the parameter.\r\n", xWriteBufferLen );
		return pdFALSE;
	}

	printf("memory total: %7d\n", configMEM_LEN);
	printf("\nARM heap status\n");
	printf("total: %7d\n", xTotalHeapBytes);
	printf("used:  %7d\n", (xTotalHeapBytes - xFreeBytesRemaining));
	printf("free:  %7d\n", xFreeBytesRemaining);
	pcWriteBuffer[0]=0;
	if (strncmp(param, "on", 2) == 0)
		mleak_set_enable(1);
	else if(strncmp(param, "off", 3) == 0)
		mleak_set_enable(0);
	else {
		printf("wrong parameter!\n");
	}
	return 0;
}
void memleak2_cmd_init(void)
{
	FreeRTOS_CLIRegisterCommand(&xMemLeak2Command);
}
#endif

void *pvPortMalloc( size_t xWantedSize )
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;
#if ENABLE_KASAN
	size_t mallocsz = xWantedSize;
#endif

#ifdef CONFIG_MEMORY_LEAK
	char *taskname = pcTaskGetName(NULL);
	int MemTaskNum = 0;
	int len = 0;
//	int MemTaskNum = uxTaskGetTaskNumber(xTaskGetCurrentTaskHandle());
	if (taskname != NULL)
		MemTaskNum = uxTaskGetTaskNumber(xTaskGetHandle(taskname));
#endif

	if (xWantedSize <= 0)
		return pvReturn;

#ifdef ARCH64
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif
	{
		/* If this is the first call to malloc then the heap will require
		   initialisation to setup the list of free blocks. */
		if ( pxEnd == NULL )
		{
			vPortDefineHeapRegions(NULL);
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if ( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if ( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
#ifdef CONFIG_MEMORY_LEAK
				if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
					MemLeak_t[0].Flag = 1;
					MemLeak_t[0].TaskNum = 0;
					MemLeak_t[0].WantSize = xWantedSize;
					MemLeak_t[0].WantTotalSize += xWantedSize;
					MemLeak_t[0].MallocCount ++;
					strncpy(MemLeak_t[0].TaskName, "not_in_task", 20);
					MemLeak_t[0].TaskName[sizeof(MemLeak_t[0].TaskName) - 1] = '\0';
				} else {
					if (taskname != NULL) {
						MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
						MemLeak_t[MemTaskNum].WantSize = xWantedSize;
						MemLeak_t[MemTaskNum].WantTotalSize += xWantedSize;
						MemLeak_t[MemTaskNum].MallocCount ++;
						len = sizeof(MemLeak_t[MemTaskNum].TaskName) > strlen(taskname) ? strlen(taskname) : sizeof(MemLeak_t[MemTaskNum].TaskName);
						strncpy(MemLeak_t[MemTaskNum].TaskName, taskname, len);
						MemLeak_t[MemTaskNum].TaskName[sizeof(MemLeak_t[MemTaskNum].TaskName) - 1] = '\0';
					}
				}
#endif
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if ( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while( ( pxBlock->xBlockSize < xWantedSize ) && ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if ( pxBlock != pxEnd )
				{
					/* Return the memory space pointed to - jumping over the
					BlockLink_t structure at its start. */
					pvReturn = ( void * ) ( ( ( uint8_t * ) pxPreviousBlock->pxNextFreeBlock ) + xHeapStructSize );

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;

					/* If the block is larger than required it can be split into
					two. */
					if( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
#if CONFIG_MEMORY_LEAK2 && CONFIG_STACK_TRACE
	if (pvReturn) on_malloc(pvReturn,xWantedSize);
	else {
		printf("malloc size %lu fail\n", xWantedSize);
		dump_stack();
		mleak_set_enable(0);
	}
#endif
#ifdef ARCH64
	portIRQ_RESTORE(flags);
#else
	( void ) xTaskResumeAll();
#endif

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif
#if ENABLE_KASAN
	if ( pvReturn ) {
		BlockLink_t *pxTmp = (BlockLink_t *)((( uint8_t * )pvReturn)-xHeapStructSize);
		kasan_poison((void*)pxTmp,xHeapStructSize,KASAN_MALLOC_REDZONE);
		kasan_unpoison(pvReturn,mallocsz);
	}
#endif

	return pvReturn;
}
static char* _pxGetAlignedAddr(BlockLink_t *pxBlock, size_t xWantedSize,
			       size_t xAlignMsk, int alloc)
{
	char *p = (char*)pxBlock;
	char *end = p+pxBlock->xBlockSize;
	if (pxBlock->pxNextFreeBlock == NULL)
		return NULL;
	if (xAlignMsk < portBYTE_ALIGNMENT_MASK)
		xAlignMsk = portBYTE_ALIGNMENT_MASK;
	if (alloc) {
		xWantedSize -= xHeapStructSize;
		p += xHeapStructSize;
	}
	p = (char*)((((unsigned long)p)+xAlignMsk)&(~xAlignMsk));
	if ((unsigned long)p >= (unsigned long)pxBlock
		&& (unsigned long)end > (unsigned long)p
		&& (unsigned long)(end-p) >= xWantedSize){
		return p;
	}else return NULL;
}
void *early_reserve_pages(size_t xWantedSize)
{
	int i=0,j;
	size_t xRegionSize;
	size_t xAddress,xEndAddr,xAlignedAddress;
	configASSERT((xWantedSize&0xFFF)==0);
	configASSERT( pxEnd == NULL );
	for (i=0; xDefRegion[i].xSizeInBytes; i++) {
		xAddress = (size_t)xDefRegion[i].pucStartAddress;
		xRegionSize = xDefRegion[i].xSizeInBytes;
		if ( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 ) {
			xAddress += ( portBYTE_ALIGNMENT - 1 );
			xAddress &= ~portBYTE_ALIGNMENT_MASK;
			xRegionSize -= xAddress - ( size_t ) xDefRegion[i].pucStartAddress;
			xDefRegion[i].pucStartAddress = (void*)xAddress;
			xDefRegion[i].xSizeInBytes = xRegionSize;
		}
		xEndAddr = xAddress + xRegionSize;
		if (xAddress&0xFFF) {
			xAlignedAddress = ((xAddress+0xFFF)&~0xFFFUL);
			if ((xAlignedAddress >= xEndAddr)
			    || (xDefRegion[MAX_REGION_CNT-1].xSizeInBytes != 0))
				continue;
			for (j=MAX_REGION_CNT-1; j>(i+1); j--) {
				xDefRegion[j] = xDefRegion[j-1];
			}
			xDefRegion[i].xSizeInBytes = xAlignedAddress-xAddress;
			xDefRegion[i+1].pucStartAddress = (void*)xAlignedAddress;
			xDefRegion[i+1].xSizeInBytes = xEndAddr-xAlignedAddress;
		} else if ((xAddress+xWantedSize) <= xEndAddr) {
			xAlignedAddress = xAddress+xWantedSize;
			xDefRegion[i].pucStartAddress = (void*)xAlignedAddress;
			if (xAlignedAddress != xEndAddr)
				xDefRegion[i].xSizeInBytes = xEndAddr-xAlignedAddress;
			else
				xDefRegion[i].xSizeInBytes = 1;
			return (void*)xAddress;
		}
	}
	return NULL;
}
void *pvPortMallocRsvAlign( size_t xWantedSize , size_t xAlignMsk)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;
#if ENABLE_KASAN
	size_t mallocsz = xWantedSize;
#endif


	if (xWantedSize <= 0)
		return pvReturn;
	configASSERT(((xAlignMsk+1)&xAlignMsk)==0);

#ifdef ARCH64
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif
	{
		/* If this is the first call to malloc then the heap will require
		   initialisation to setup the list of free blocks. */
		if ( pxEnd == NULL )
		{
			vPortDefineHeapRegions(NULL);
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if ( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if ( xWantedSize > 0 )
			{
				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if ( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if ( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while ( ( ( pvReturn=_pxGetAlignedAddr(pxBlock,xWantedSize,xAlignMsk,0)) == NULL )
					&& ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if ( pvReturn )
				{
					BlockLink_t *pxTmp = (BlockLink_t *)((( uint8_t * )pvReturn));
					long tmplen=( uint8_t * )pxTmp-( uint8_t * )pxBlock;
					long tmplen2 = pxBlock->xBlockSize - tmplen;
					pxTmp->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
					pxTmp->xBlockSize = tmplen2;

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					if ( tmplen >= (long)xHeapStructSize ) {
						pxBlock->xBlockSize = tmplen;
						pxPreviousBlock = pxBlock;
					} else {
						pxPreviousBlock->pxNextFreeBlock = pxTmp->pxNextFreeBlock;
					}
					pxBlock = pxTmp;

					/* If the block is larger than required it can be split into
					two. */
					if ( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if ( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
#ifdef ARCH64
	portIRQ_RESTORE(flags);
#else
	( void ) xTaskResumeAll();
#endif
#if ENABLE_KASAN
	if ( pvReturn ) {
		kasan_unpoison(pvReturn,mallocsz);
	}
#endif

	return pvReturn;
}
void *pvPortMallocAlign( size_t xWantedSize , size_t xAlignMsk)
{
	BlockLink_t *pxBlock, *pxPreviousBlock, *pxNewBlockLink;
	void *pvReturn = NULL;
	unsigned long flags;
#if ENABLE_KASAN
	size_t mallocsz = xWantedSize;
#endif

#ifdef CONFIG_MEMORY_LEAK
	char *taskname = pcTaskGetName(NULL);
	int MemTaskNum = 0;
	int len = 0;
//	int MemTaskNum = uxTaskGetTaskNumber(xTaskGetCurrentTaskHandle());
	if (taskname != NULL)
		MemTaskNum = uxTaskGetTaskNumber(xTaskGetHandle(taskname));
#endif

	if (xWantedSize <= 0)
		return pvReturn;
	configASSERT(((xAlignMsk+1)&xAlignMsk)==0);

#ifdef ARCH64
	portIRQ_SAVE(flags);
#else
	vTaskSuspendAll();
#endif
	{
		/* If this is the first call to malloc then the heap will require
		   initialisation to setup the list of free blocks. */
		if ( pxEnd == NULL )
		{
			vPortDefineHeapRegions(NULL);
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		/* Check the requested block size is not so large that the top bit is
		set.  The top bit of the block size member of the BlockLink_t structure
		is used to determine who owns the block - the application or the
		kernel, so it must be free. */
		if ( ( xWantedSize & xBlockAllocatedBit ) == 0 )
		{
			/* The wanted size is increased so it can contain a BlockLink_t
			structure in addition to the requested amount of bytes. */
			if ( xWantedSize > 0 )
			{
				xWantedSize += xHeapStructSize;

				/* Ensure that blocks are always aligned to the required number
				of bytes. */
				if ( ( xWantedSize & portBYTE_ALIGNMENT_MASK ) != 0x00 )
				{
					/* Byte alignment required. */
					xWantedSize += ( portBYTE_ALIGNMENT - ( xWantedSize & portBYTE_ALIGNMENT_MASK ) );
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
#ifdef CONFIG_MEMORY_LEAK
				if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
					MemLeak_t[0].Flag = 1;
					MemLeak_t[0].TaskNum = 0;
					MemLeak_t[0].WantSize = xWantedSize;
					MemLeak_t[0].WantTotalSize += xWantedSize;
					MemLeak_t[0].MallocCount ++;
					strncpy(MemLeak_t[0].TaskName, "not_in_task", 20);
					MemLeak_t[0].TaskName[sizeof(MemLeak_t[0].TaskName) - 1] = '\0';
				} else {
					if (taskname != NULL) {
						MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
						MemLeak_t[MemTaskNum].WantSize = xWantedSize;
						MemLeak_t[MemTaskNum].WantTotalSize += xWantedSize;
						MemLeak_t[MemTaskNum].MallocCount ++;
						len = sizeof(MemLeak_t[MemTaskNum].TaskName) > strlen(taskname) ? strlen(taskname) : sizeof(MemLeak_t[MemTaskNum].TaskName);
						strncpy(MemLeak_t[MemTaskNum].TaskName, taskname, len);
						MemLeak_t[MemTaskNum].TaskName[sizeof(MemLeak_t[MemTaskNum].TaskName) - 1] = '\0';
					}
				}
#endif
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}

			if ( ( xWantedSize > 0 ) && ( xWantedSize <= xFreeBytesRemaining ) )
			{
				/* Traverse the list from the start	(lowest address) block until
				one	of adequate size is found. */
				pxPreviousBlock = &xStart;
				pxBlock = xStart.pxNextFreeBlock;
				while ( ( ( pvReturn=_pxGetAlignedAddr(pxBlock,xWantedSize,xAlignMsk,1)) == NULL )
					&& ( pxBlock->pxNextFreeBlock != NULL ) )
				{
					pxPreviousBlock = pxBlock;
					pxBlock = pxBlock->pxNextFreeBlock;
				}

				/* If the end marker was reached then a block of adequate size
				was	not found. */
				if ( pvReturn )
				{
					BlockLink_t *pxTmp = (BlockLink_t *)((( uint8_t * )pvReturn)-xHeapStructSize);

					/* This block is being returned for use so must be taken out
					of the list of free blocks. */
					if ((unsigned long)pxTmp > (unsigned long)pxBlock) {
						long tmplen=( uint8_t * )pxTmp-( uint8_t * )pxBlock;
						configASSERT(tmplen>=(long)xHeapStructSize);
						pxTmp->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
						pxTmp->xBlockSize = pxBlock->xBlockSize - tmplen;
						pxBlock->xBlockSize = tmplen;
						pxPreviousBlock = pxBlock;
						pxBlock = pxTmp;
					} else {
						configASSERT(pxTmp == pxBlock);
						pxPreviousBlock->pxNextFreeBlock = pxBlock->pxNextFreeBlock;
					}

					/* If the block is larger than required it can be split into
					two. */
					if ( ( pxBlock->xBlockSize - xWantedSize ) > heapMINIMUM_BLOCK_SIZE )
					{
						/* This block is to be split into two.  Create a new
						block following the number of bytes requested. The void
						cast is used to prevent byte alignment warnings from the
						compiler. */
						pxNewBlockLink = ( void * ) ( ( ( uint8_t * ) pxBlock ) + xWantedSize );

						/* Calculate the sizes of two blocks split from the
						single block. */
						pxNewBlockLink->xBlockSize = pxBlock->xBlockSize - xWantedSize;
						pxBlock->xBlockSize = xWantedSize;

						/* Insert the new block into the list of free blocks. */
						prvInsertBlockIntoFreeList( ( pxNewBlockLink ) );
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					xFreeBytesRemaining -= pxBlock->xBlockSize;

					if ( xFreeBytesRemaining < xMinimumEverFreeBytesRemaining )
					{
						xMinimumEverFreeBytesRemaining = xFreeBytesRemaining;
					}
					else
					{
						mtCOVERAGE_TEST_MARKER();
					}

					/* The block is being returned - it is allocated and owned
					by the application and has no "next" block. */
					pxBlock->xBlockSize |= xBlockAllocatedBit;
					pxBlock->pxNextFreeBlock = NULL;
				}
				else
				{
					mtCOVERAGE_TEST_MARKER();
				}
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}

		traceMALLOC( pvReturn, xWantedSize );
	}
#if CONFIG_MEMORY_LEAK2 && CONFIG_STACK_TRACE
	if (pvReturn) on_malloc(pvReturn,xWantedSize);
	else {
		printf("malloc size %lu fail\n", xWantedSize);
		dump_stack();
		mleak_set_enable(0);
	}
#endif
#ifdef ARCH64
	portIRQ_RESTORE(flags);
#else
	( void ) xTaskResumeAll();
#endif

	#if( configUSE_MALLOC_FAILED_HOOK == 1 )
	{
		if ( pvReturn == NULL )
		{
			extern void vApplicationMallocFailedHook( void );
			vApplicationMallocFailedHook();
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
	#endif
#if ENABLE_KASAN
	if ( pvReturn ) {
		BlockLink_t *pxTmp = (BlockLink_t *)((( uint8_t * )pvReturn)-xHeapStructSize);
		kasan_poison((void*)pxTmp,xHeapStructSize,KASAN_MALLOC_REDZONE);
		kasan_unpoison(pvReturn,mallocsz);
	}
#endif

	return pvReturn;
}
/*-----------------------------------------------------------*/

void vPortFree( void *pv )
{
	uint8_t *puc = ( uint8_t * ) pv;
	BlockLink_t *pxLink;
	unsigned long flags;
#ifdef CONFIG_MEMORY_LEAK
	char *taskname = pcTaskGetName(NULL);
	int MemTaskNum = 0;
	int len = 0;
	//int MemTaskNum = uxTaskGetTaskNumber(xTaskGetCurrentTaskHandle());
	if (taskname != NULL)
		MemTaskNum = uxTaskGetTaskNumber(xTaskGetHandle(taskname));
#endif

	if ( pv != NULL )
	{
		/* The memory being freed will have an BlockLink_t structure immediately
		before it. */
		puc -= xHeapStructSize;

		/* This casting is to keep the compiler from issuing warnings. */
		pxLink = ( void * ) puc;

		/* Check the block is actually allocated. */
		configASSERT( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 );
		configASSERT( pxLink->pxNextFreeBlock == NULL );

#ifdef CONFIG_MEMORY_LEAK
		if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
			MemLeak_t[0].FreeSize = pxLink->xBlockSize;
			MemLeak_t[0].FreeTotalSize += pxLink->xBlockSize;
			MemLeak_t[0].FreeCount ++;
		} else {
			if (taskname != NULL) {
				MemLeak_t[MemTaskNum].TaskNum = MemTaskNum;
				MemLeak_t[MemTaskNum].FreeSize = pxLink->xBlockSize;
				MemLeak_t[MemTaskNum].FreeTotalSize += pxLink->xBlockSize;
				MemLeak_t[MemTaskNum].FreeCount ++;
				len = sizeof(MemLeak_t[MemTaskNum].TaskName) > strlen(taskname) ? strlen(taskname) : sizeof(MemLeak_t[MemTaskNum].TaskName);
				strncpy(MemLeak_t[MemTaskNum].TaskName, taskname, len);
				MemLeak_t[MemTaskNum].TaskName[sizeof(MemLeak_t[MemTaskNum].TaskName) - 1] = '\0';
			}
		}
#endif
		if( ( pxLink->xBlockSize & xBlockAllocatedBit ) != 0 )
		{
			if( pxLink->pxNextFreeBlock == NULL )
			{
				/* The block is being returned to the heap - it is no longer
				allocated. */
				pxLink->xBlockSize &= ~xBlockAllocatedBit;
#if ENABLE_KASAN
				kasan_poison(pv,pxLink->xBlockSize-xHeapStructSize,KASAN_MALLOC_FREE);
#endif

#ifdef ARCH64
				portIRQ_SAVE(flags);
#else
				vTaskSuspendAll();
#endif
#if CONFIG_MEMORY_LEAK2 && CONFIG_STACK_TRACE
				on_free(pv);
#endif
				{
					/* Add this block to the list of free blocks. */
					xFreeBytesRemaining += pxLink->xBlockSize;
					traceFREE( pv, pxLink->xBlockSize );
					prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
				}
#ifdef ARCH64
				portIRQ_RESTORE(flags);
#else
				( void ) xTaskResumeAll();
#endif
			}
			else
			{
				mtCOVERAGE_TEST_MARKER();
			}
		}
		else
		{
			mtCOVERAGE_TEST_MARKER();
		}
	}
}
/*-----------------------------------------------------------*/

size_t xPortGetFreeHeapSize( void )
{
	return xFreeBytesRemaining;
}
size_t xPortGetTotalHeapSize( void )
{
	return xTotalHeapBytes;
}
/*-----------------------------------------------------------*/

size_t xPortGetMinimumEverFreeHeapSize( void )
{
	return xMinimumEverFreeBytesRemaining;
}
/*-----------------------------------------------------------*/

static void prvInsertBlockIntoFreeList( BlockLink_t *pxBlockToInsert )
{
BlockLink_t *pxIterator;
uint8_t *puc;

	/* Iterate through the list until a block is found that has a higher address
	than the block being inserted. */
	for( pxIterator = &xStart; pxIterator->pxNextFreeBlock < pxBlockToInsert; pxIterator = pxIterator->pxNextFreeBlock )
	{
		/* Nothing to do here, just iterate to the right position. */
	}

	/* Do the block being inserted, and the block it is being inserted after
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxIterator;
	if( ( puc + pxIterator->xBlockSize ) == ( uint8_t * ) pxBlockToInsert )
	{
		pxIterator->xBlockSize += pxBlockToInsert->xBlockSize;
		pxBlockToInsert = pxIterator;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}

	/* Do the block being inserted, and the block it is being inserted before
	make a contiguous block of memory? */
	puc = ( uint8_t * ) pxBlockToInsert;
	if( ( puc + pxBlockToInsert->xBlockSize ) == ( uint8_t * ) pxIterator->pxNextFreeBlock )
	{
		if( pxIterator->pxNextFreeBlock != pxEnd )
		{
			/* Form one big block from the two blocks. */
			pxBlockToInsert->xBlockSize += pxIterator->pxNextFreeBlock->xBlockSize;
			pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock->pxNextFreeBlock;
		}
		else
		{
			pxBlockToInsert->pxNextFreeBlock = pxEnd;
		}
	}
	else
	{
		pxBlockToInsert->pxNextFreeBlock = pxIterator->pxNextFreeBlock;
	}

	/* If the block being inserted plugged a gab, so was merged with the block
	before and the block after, then it's pxNextFreeBlock pointer will have
	already been set, and should not be set here as that would make it point
	to itself. */
	if( pxIterator != pxBlockToInsert )
	{
		pxIterator->pxNextFreeBlock = pxBlockToInsert;
	}
	else
	{
		mtCOVERAGE_TEST_MARKER();
	}
}
/*-----------------------------------------------------------*/

void vPortDefineHeapRegions( const HeapRegion_t * const pRegions )
{
BlockLink_t *pxFirstFreeBlockInRegion = NULL, *pxPreviousFreeBlock;
size_t xAlignedHeap;
size_t xTotalRegionSize, xTotalHeapSize = 0;
BaseType_t xDefinedRegions = 0;
size_t xAddress;
const HeapRegion_t *pxHeapRegion;
const HeapRegion_t *pxHeapRegions=pRegions;

	/* Can only call once! */
	configASSERT( pxEnd == NULL );

	if (!pxHeapRegions)
		pxHeapRegions = xDefRegion;

	pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );

	while( pxHeapRegion->xSizeInBytes > 0 )
	{
		xTotalRegionSize = pxHeapRegion->xSizeInBytes;

		/* Ensure the heap region starts on a correctly aligned boundary. */
		xAddress = ( size_t ) pxHeapRegion->pucStartAddress;
		if( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
		{
			xAddress += ( portBYTE_ALIGNMENT - 1 );
			xAddress &= ~portBYTE_ALIGNMENT_MASK;

			/* Adjust the size for the bytes lost to alignment. */
			xTotalRegionSize -= xAddress - ( size_t ) pxHeapRegion->pucStartAddress;
		}
		if (xTotalRegionSize < 2*xHeapStructSize) {
			xDefinedRegions++;
			pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
			continue;
		}

		xAlignedHeap = xAddress;
#if ENABLE_KASAN
		kasan_poison((void*)xAddress,xTotalRegionSize,KASAN_MALLOC_REDZONE);
#endif

		/* Set xStart if it has not already been set. */
		if( xDefinedRegions == 0 )
		{
			/* xStart is used to hold a pointer to the first item in the list of
			free blocks.  The void cast is used to prevent compiler warnings. */
			xStart.pxNextFreeBlock = ( BlockLink_t * ) xAlignedHeap;
			xStart.xBlockSize = ( size_t ) 0;
		}
		else
		{
			/* Should only get here if one region has already been added to the
			heap. */
			configASSERT( pxEnd != NULL );

			/* Check blocks are passed in with increasing start addresses. */
			configASSERT( xAddress > ( size_t ) pxEnd );
		}

		/* Remember the location of the end marker in the previous region, if
		any. */
		pxPreviousFreeBlock = pxEnd;

		/* pxEnd is used to mark the end of the list of free blocks and is
		inserted at the end of the region space. */
		xAddress = xAlignedHeap + xTotalRegionSize;
		xAddress -= xHeapStructSize;
		xAddress &= ~portBYTE_ALIGNMENT_MASK;
		pxEnd = ( BlockLink_t * ) xAddress;
		pxEnd->xBlockSize = 0;
		pxEnd->pxNextFreeBlock = NULL;

		/* To start with there is a single free block in this region that is
		sized to take up the entire heap region minus the space taken by the
		free block structure. */
		pxFirstFreeBlockInRegion = ( BlockLink_t * ) xAlignedHeap;
		pxFirstFreeBlockInRegion->xBlockSize = xAddress - ( size_t ) pxFirstFreeBlockInRegion;
		pxFirstFreeBlockInRegion->pxNextFreeBlock = pxEnd;

		/* If this is not the first region that makes up the entire heap space
		then link the previous region to this region. */
		if( pxPreviousFreeBlock != NULL )
		{
			pxPreviousFreeBlock->pxNextFreeBlock = pxFirstFreeBlockInRegion;
		}

		xTotalHeapSize += pxFirstFreeBlockInRegion->xBlockSize;

		/* Move onto the next HeapRegion_t structure. */
		xDefinedRegions++;
		pxHeapRegion = &( pxHeapRegions[ xDefinedRegions ] );
	}

	xMinimumEverFreeBytesRemaining = xTotalHeapSize;
	xFreeBytesRemaining = xTotalHeapSize;
	xTotalHeapBytes = xTotalHeapSize;

	/* Check something was actually defined before it is accessed. */
	configASSERT( xTotalHeapSize );

	/* Work out the position of the top bit in a size_t variable. */
	xBlockAllocatedBit = ( ( size_t ) 1 ) << ( ( sizeof( size_t ) * heapBITS_PER_BYTE ) - 1 );
}

void vPortAddHeapRegion( uint8_t *pucStartAddress, size_t xSizeInBytes )
{
	BlockLink_t *pxLink = NULL, *pxPreviousFreeBlock;
	size_t xAlignedHeap;
	size_t xTotalRegionSize;
	size_t xAddress;
	HeapRegion_t region[2];
	int bneedsus=0;

	if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
		bneedsus=1;
	if (bneedsus)vTaskSuspendAll();
	if (!pxEnd) {
		memset(region, 0, sizeof(region));
		region[0].pucStartAddress=pucStartAddress;
		region[0].xSizeInBytes=xSizeInBytes;
		vPortDefineHeapRegions(region);
	} else {
		xAddress = ( size_t ) pucStartAddress;
		xTotalRegionSize = xSizeInBytes;
		if ( ( xAddress & portBYTE_ALIGNMENT_MASK ) != 0 )
		{
			xAddress += ( portBYTE_ALIGNMENT - 1 );
			xAddress &= ~portBYTE_ALIGNMENT_MASK;
			xTotalRegionSize -= xAddress - ( size_t ) pucStartAddress;
		} else {
			mtCOVERAGE_TEST_MARKER();
		}
#if ENABLE_KASAN
		kasan_poison((void*)xAddress,xTotalRegionSize,KASAN_MALLOC_REDZONE);
#endif

		if (xTotalRegionSize>heapMINIMUM_BLOCK_SIZE) {
			xAlignedHeap = xAddress;
			pxLink = ( BlockLink_t * ) xAlignedHeap;

			if (pxLink <= pxEnd) {
				pxLink->xBlockSize = ( size_t ) xTotalRegionSize;
				xFreeBytesRemaining += pxLink->xBlockSize;
				xTotalHeapBytes += pxLink->xBlockSize;
				prvInsertBlockIntoFreeList( ( ( BlockLink_t * ) pxLink ) );
			} else {
				pxPreviousFreeBlock = pxEnd;
				xAddress = xAlignedHeap + xTotalRegionSize;
				xAddress -= xHeapStructSize;
				xAddress &= ~portBYTE_ALIGNMENT_MASK;
				pxEnd = ( BlockLink_t * ) xAddress;
				pxEnd->xBlockSize = 0;
				pxEnd->pxNextFreeBlock = NULL;

				pxPreviousFreeBlock->pxNextFreeBlock = pxLink;
				pxLink->xBlockSize = xAddress - ( size_t ) pxLink;
				pxLink->pxNextFreeBlock = pxEnd;
				xFreeBytesRemaining += pxLink->xBlockSize;
				xTotalHeapBytes += pxLink->xBlockSize;
			}
		} else {
			mtCOVERAGE_TEST_MARKER();
		}
	}
	if (bneedsus)xTaskResumeAll();
}

void* xPortRealloc(void *ptr, size_t size)
{
	void *p = NULL;
	size_t old_len = 0;
	size_t len = 0;

	if (ptr) {
		BlockLink_t *pxTmp = (BlockLink_t *)((( uint8_t * )ptr)-xHeapStructSize);
		old_len = pxTmp->xBlockSize - xHeapStructSize;
		len = old_len < size ? old_len : size;
		if (!size) {
			vPortFree(ptr);
			return NULL;
		}

		p = pvPortMalloc(size);
		if (p) {
			memcpy(p, ptr, len);
			if (size>len) memset((char*)p+len, 0, size-len);
		}
		vPortFree(ptr);
	}
	else {
		p = pvPortMalloc(size);
	}

	return p;
}

int vPrintFreeListAfterMallocFail(void)
{
	BlockLink_t *pxIterator;
	int total_free_size = 0;

	for ( pxIterator = &xStart; pxIterator != pxEnd; pxIterator = pxIterator->pxNextFreeBlock )
	{
		printf("the address: %p, len: %d\n", pxIterator, (int)(pxIterator->xBlockSize));
#ifdef ARCH64
		/*print_lastword_crash("the address: %p, len: 0x%x\n", pxIterator, (int)(pxIterator->xBlockSize));*/
#endif
		total_free_size += (pxIterator->xBlockSize);
	}
	printf("the total free size: %d\n", total_free_size);
#ifdef ARCH64
	/*print_lastword_crash("the total free size: %d\n", total_free_size);*/
#endif

	return 0;
}
