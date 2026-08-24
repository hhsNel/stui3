#include "stui3/module.h"

#include <stdio.h>

char hello_world[] = "Hello, World!";

stui3_module_symbol symbols[] = {
	"hello_world",
};
void *pointers[] = {
	&hello_world,
};

void on_load() {
	fprintf(stderr, "dummy 2 module loaded!\n");
}

void on_unload() {
	fprintf(stderr, "dummy 2 module unloaded!\n");
}

struct stui3_module_description stui3_module = {
	.magic = { 0x9A, 0x87, 0x6E, 0x95 },
	.minimum_abi = 1,
	.deprecation_abi = ABI_VERSION + 1,
	.name = "dummy2",
	.description = "This is another module provides a \"hello_world\" symbol which is a simple C string",
	.symbols = symbols,
	.pointers = pointers,
	.num_exports = sizeof(symbols)/sizeof(*symbols),
	.load_hook = on_load,
	.unload_hook = on_unload,
};

