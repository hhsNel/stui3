#pragma once
#include <stdio.h>
#include <stdlib.h>

#define SKIP_UNLESS(feature) \
	do { \
		if (!(feature)) { \
			printf("SKIP %s (disabled: %s)\n", __FILE__, #feature); \
			exit(0); \
		} \
	} while (0)

