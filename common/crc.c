#include "crc.h"

#include "protocol.h"

static uint8_t crc8_lut[256];
static int crc8_initialized = 0;
static uint32_t crc32_lut[256];
static int crc32_initialized = 0;

static void init_crc8_lut();
static void init_crc32_lut();

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

uint32_t
protocol_crc32(uint8_t const *data, size_t len) {
	uint32_t crc = CRC32_INITIAL;

	if(! crc32_initialized) {
		init_crc32_lut();
		crc32_initialized = 1;
	}

	while(len--) {
		crc = crc32_lut[(uint8_t)(crc ^ *(data++))] ^ (crc >> 8);
	}

	return crc ^ CRC32_XOR;
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

static void
init_crc32_lut() {
	unsigned int i, j;
	uint32_t crc;

	for(i = 0; i < 256; ++i) {
		crc = i;
		for(j = 0; j < 8; ++j) {
			if(crc & 0x00000001) {
				crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
			} else {
				crc = crc >> 1;
			}
		}
		crc32_lut[i] = crc;
	}
}

