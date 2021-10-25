#include "aml_strlen.h"

int strlen(const char *s)
{
	int len = 0;

	while (*s++)
		len++;

	return len;
}

