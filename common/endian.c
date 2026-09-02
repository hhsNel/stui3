#include "endian.h"

#include <stdint.h>

int
is_little_endian() {
	uint16_t test;

	test = 1;
	if(*(uint8_t *)&test == (uint8_t)test) {
		return 1;
	} else {
		return 0;
	}
}

int
is_big_endian() {
	return !is_little_endian();
}

