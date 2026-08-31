#include "stui3/module.h"

#include <stdio.h>

static char hello_world[] = "Hello, World!";

static stui3_module_symbol symbols[] = {
	"hello_world",
};
static void *pointers[] = {
	&hello_world,
};

static int on_load() {
	fprintf(stderr, "dummy 2 module loaded!\n");
	return 0;
}

static int command(char const *const in, char *const out, size_t const out_sz) {
	(void)in;
	if(out_sz) *out = '\0';
	return 1;
}

static void on_unload() {
	fprintf(stderr, "dummy 2 module unloaded!\n");
}

struct stui3_module_description STUI3_MODULE_NAME(stui3_module) = {
	.magic = { 0x9A, 0x87, 0x6E, 0x95 },
	.minimum_abi = 5,
	.deprecation_abi = ABI_VERSION + 1,
	.name = "dummy2",
	.description = "This is another module provides a \"hello_world\" symbol which is a simple C string",
	.symbols = symbols,
	.pointers = pointers,
	.num_exports = sizeof(symbols)/sizeof(*symbols),
	.load_hook = on_load,
	.command = command,
	.unload_hook = on_unload,
};

