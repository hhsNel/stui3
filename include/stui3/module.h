#ifndef STUI3_MODULE_H
#define STUI3_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#define STUI3_MODULE_SYMBOL_LENGTH 32
#define STUI3_MODULE_NAME_LENGTH 32

#include <stdint.h>
#include <stddef.h>

typedef char stui3_module_symbol[STUI3_MODULE_SYMBOL_LENGTH];
typedef uint32_t stui3_module_abi;

struct stui3_module_description {
	uint8_t magic[4]; /* 0x9A 0x87 0x6E 0x95 */
	stui3_module_abi minimum_abi;
	stui3_module_abi deprecation_abi;
	char name[STUI3_MODULE_NAME_LENGTH];
	char *description;
	stui3_module_symbol *symbols;
	void **pointers;
	size_t num_exports;
	void (*load_hook)();
	void (*unload_hook)();
};

/* struct stui3_module_description stui3_module; */

#ifdef __cplusplus
};
#endif

#endif

