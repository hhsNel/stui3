#include "symbols/symbol-table.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main() {
	char buf[2048];
	char arg[64];
	int argi;
	char c;
	int providers[8];
	int i, n;
	struct module_info minfo;
	char desc_buf[2048];
	stui3_module_symbol symbol_buf[256];

	init_symbols();
	atexit(shutdown_symbols);

	minfo.description = desc_buf;
	minfo.description_count = sizeof(desc_buf)/sizeof(*desc_buf);
	minfo.exports = symbol_buf;
	minfo.export_count = sizeof(symbol_buf)/sizeof(*symbol_buf);
	minfo.export_arg = 0;

	puts("ready");

	while(1) {
		if(! scanf("%2048[^\n]", buf)) exit(1);
		if(! scanf("%c", &c)) exit(1);

#define ERROR() \
	do { puts("error"); continue; } while(0);

		if(strncmp(buf, "help", 4) == 0) {
			puts("help|lookups <symbol_name>|lookupm <module_name>|load <module_name>|unload <module_id>|providers <symbol_name>|set <symbol_name> <module_id>|num|info <module_id>|command <module_id> <cmd_string>|exit");
		} else if(strncmp(buf, "lookups", 7) == 0) {
			sscanf(buf, "lookups %64s", arg);
			printf("ok %p\n", lookup_symbol(arg));
		} else if(strncmp(buf, "lookupm", 7) == 0) {
			sscanf(buf, "lookupm %64s", arg);
			printf("ok %d\n", lookup_module(arg));
		} else if(strncmp(buf, "load", 4) == 0) {
			sscanf(buf, "load %64s", arg);
			printf("ok %d\n", load_module(arg));
		} else if(strncmp(buf, "unload", 6) == 0) {
			sscanf(buf, "unload %d", &argi);
			unload_module(argi);
			printf("ok\n");
		} else if(strncmp(buf, "exit", 4) == 0) {
			exit(0);
		} else if(strncmp(buf, "providers", 9) == 0) {
			sscanf(buf, "providers %64s", arg);
			printf("ok");
			n = (int)symbol_providers(arg, providers, sizeof(providers)/sizeof(*providers));
			for(i = 0; i < n; ++i) {
				printf(" %d", providers[i]);
			}
			printf("\n");
		} else if(strncmp(buf, "set", 3) == 0) {
			sscanf(buf, "set %64s %d", arg, &argi);
			printf("ok %d\n", set_symbol_provider(arg, argi));
		} else if(strncmp(buf, "num", 3) == 0) {
			printf("ok %zu\n", module_count());
		} else if(strncmp(buf, "info", 4) == 0) {
			sscanf(buf, "info %d", &argi);
			module_info(argi, &minfo);
			puts("ok");
			printf("\tid: %d\n", minfo.module_id);
			printf("\tname: %s\n", minfo.name);
			printf("\tdescription: %s\n", desc_buf);
			printf("\texports (%zu):\n", minfo.export_arg);
			for(i = 0; (size_t)i < minfo.export_arg; ++i) {
				printf("\t\t- %s\n", symbol_buf[i]);
			}
			minfo.export_arg = 0;
		} else if(strncmp(buf, "command", 7) == 0) {
			sscanf(buf, "command %d %64s", &argi, arg);
			printf("ok %d\n", module_command(argi, arg, desc_buf, sizeof(desc_buf)));
			printf("feedback: %s\n", desc_buf);
		} else {
			printf("unknown command: %s\n", buf);
		}
	}
}

