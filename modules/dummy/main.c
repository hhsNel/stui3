#include "stui3/module.h"

char hello_world[] = "Hello, World!";

stui3_module_symbol symbols[] = {
	"hello_world",
};
void *pointers[] = {
	&hello_world,
};

struct stui3_module_description stui3_module = {
	.magic = { 0x9A, 0x87, 0x6E, 0x95 },
	.minimum_abi = 0,
	.deprecation_abi = ABI_VERSION + 1,
	.name = "dummy.so",
	.description = "This module provides a \"hello_world\" symbol which is a simple C string",
	.symbols = symbols,
	.pointers = pointers,
	.num_exports = sizeof(symbols)/sizeof(*symbols),
};

