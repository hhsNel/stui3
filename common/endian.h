#ifndef COMMON_ENDIAN_H
#define COMMON_ENDIAN_H

#include <stdint.h>

int is_little_endian();
int is_big_endian();
uint16_t byteswap16(uint16_t const orig);
uint32_t byteswap32(uint32_t const orig);
uint64_t byteswap64(uint64_t const orig);

#endif

