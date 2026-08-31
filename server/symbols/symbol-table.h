#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "stui3/module.h"

struct module_info {
	int module_id;
	char name[STUI3_MODULE_NAME_LENGTH];

	char *description;
	size_t description_count;
	/* if description_count is 0 and description is NULL, description_count
	 is set to the length of the description.  Otherwise, description is
	 treated as a description_count-long buffer and filled in */

	stui3_module_symbol *exports;
	size_t export_count;
	size_t export_arg;
	/* same as with the description, except the exports are filled in starting
	 from index export_arg, then the number of exports written is reported
	 in export_arg */
};

int init_symbols();
void shutdown_symbols();
void *lookup_symbol(char const *const symbol_name);
int lookup_module(char const *const module_name);
int load_module(char const *const module_name);
void unload_module(int const module_id);
size_t symbol_providers(char const *const symbol_name, int *const mod_arr, size_t const arr_size);
int set_symbol_provider(char const *const symbol_name, int const module_id);
size_t module_count();
void module_info(int const module_id, struct module_info *const arg);
int module_command(int const module_id, char const *const in, char *const out, size_t const out_sz);

#endif

