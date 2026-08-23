#include "symbols/symbol-table.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	char buf[2048];
	char arg[64];
	int argi;
	char c;

	init_symbols();
	atexit(shutdown_symbols);

	puts("ready");

	while(1) {
		if(! scanf("%2048[^\n]", buf)) exit(1);
		scanf("%c", &c);

#define ERROR() \
	do { puts("error"); continue; } while(0);

		if(strncmp(buf, "lookups", 7) == 0) {
			sscanf(buf, "lookups %.64s", arg);
			printf("ok %p\n", lookup_symbol(arg));
		} else if(strncmp(buf, "lookupm", 7) == 0) {
			sscanf(buf, "lookupm %.64s", arg);
			printf("ok %d\n", lookup_module(arg));
		} else if(strncmp(buf, "load", 4) == 0) {
			sscanf(buf, "load %.64s", arg);
			printf("ok %d\n", load_module(arg));
		} else if(strncmp(buf, "unload", 6) == 0) {
			sscanf(buf, "unload %d", &argi);
			unload_module(argi);
			printf("ok\n");
		} else if(strncmp(buf, "exit", 4) == 0) {
			exit(0);
		} else {
			printf("unknown command: %s\n", buf);
		}
	}
}

