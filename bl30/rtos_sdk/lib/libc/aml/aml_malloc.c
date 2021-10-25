#include "aml_malloc.h"
#include <FreeRTOS.h>

void *malloc(size_t size)
{
	return pvPortMalloc(size);
}

