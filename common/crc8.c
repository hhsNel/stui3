#include "crc8.h"

#include "protocol.h"

static uint8_t crc8_lut[256];
static int crc8_initialized = 0;

static void init_crc8_lut();

uint8_t
protocol_crc8(uint8_t const *data, size_t len) {
	uint8_t crc = CRC8_INITIAL;

	if(! crc8_initialized) {
		init_crc8_lut();
		crc8_initialized = 1;
	}

	while(len--) {
		crc = crc8_lut[crc ^ *(data++)];
	}

	return crc ^ CRC8_XOR;
}

static void
init_crc8_lut() {
	unsigned int i, j;
	uint8_t crc;

	for(i = 0; i < 256; ++i) {
		crc = i;
		for(j = 0; j < 8; ++j) {
			if(crc & 0x80) {
				crc = (crc << 1) ^ CRC8_POLYNOMIAL;
			} else {
				crc = crc << 1;
			}
		}
		crc8_lut[i] = crc;
	}
}

