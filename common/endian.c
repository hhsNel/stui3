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

uint16_t
byteswap16(uint16_t const orig) {
	return ((orig & 0x00FF) << 8) |
		((orig & 0xFF00) >> 8);
}

uint32_t
byteswap32(uint32_t const orig) {
	return ((orig & 0x000000FF) << 24) |
		((orig & 0x0000FF00) << 8) |
		((orig & 0x00FF0000) >> 8) |
		((orig & 0xFF000000) >> 24);
}

uint64_t
byteswap64(uint64_t const orig) {
	return ((orig & 0x00000000000000FF) << 56) |
		((orig & 0x000000000000FF00) << 40) |
		((orig & 0x0000000000FF0000) << 24) |
		((orig & 0x00000000FF000000) << 8) |
		((orig & 0x000000FF00000000) >> 8) |
		((orig & 0x0000FF0000000000) >> 24) |
		((orig & 0x00FF000000000000) >> 40) |
		((orig & 0xFF00000000000000) >> 56);
}


