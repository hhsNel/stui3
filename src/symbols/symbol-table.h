#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "stui3/module.h"

int init_symbols();
void shutdown_symbols();
void *lookup_symbol(stui3_module_symbol const symbol_name);
int lookup_module(char const *const module_name);
int load_module(char const *const module_name);
void unload_module(int module_id);
size_t symbol_providers(stui3_module_symbol const symbol_name, int *mod_arr, size_t arr_size);
int set_symbol_provider(stui3_module_symbol const symbol_name, int module_id);

#endif

