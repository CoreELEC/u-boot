#include "aml_free.h"
#include <FreeRTOS.h>

void free(void *ptr)
{
	vPortFree(ptr);
}
