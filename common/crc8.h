#ifndef CRC8_H
#define CRC8_H

#include <stdint.h>
#include <stddef.h>

uint8_t protocol_crc8(uint8_t const *data, size_t len);

#endif

