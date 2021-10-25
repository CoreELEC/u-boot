#include "aml_memcpy.h"

void *memcpy(void *dest, const void *src, size_t len)
{
	char *d = (char *)dest;
	const char *s = (const char *)src;
	uint32_t *dw;
	const uint32_t *sw;
	char *head;
	char * const tail = (char *)dest + len;
	/* Set 'body' to the last word boundary */
	uint32_t * const body = (uint32_t *)((uintptr_t)tail & ~3);

	if (((uintptr_t)dest & 3) != ((uintptr_t)src & 3))
	{
		/* Misaligned. no body, no tail. */
		head = tail;
	}
	else
	{
		/* Aligned */
		if ((uintptr_t)tail < (((uintptr_t)d + 3) & ~3))
		{
			/* len is shorter than the first word boundary */
			head = tail;
		}
		else
		{
			/* Set 'head' to the first word boundary */
			head = (char *)(((uintptr_t)d + 3) & ~3);
		}
	}

	/* Copy head */
	while (d < head)
	{
		*(d++) = *(s++);
	}

	/* Copy body */
	dw = (uint32_t *)d;
	sw = (uint32_t *)s;
	while (dw < body)
	{
		*(dw++) = *(sw++);
	}

	/* Copy tail */
	d = (char *)dw;
	s = (const char *)sw;
	while (d < tail)
	{
		*(d++) = *(s++);
	}

	return dest;
}

