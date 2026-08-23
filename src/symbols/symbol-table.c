#include "symbol-table.h"

#include "util.h"

#define MODULE_PATH_LEN MIN(PATH_MAX, 256)
#define MODULE_PATH "."

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>

enum module_state {
	MODULE_BUILTIN,
	MODULE_SCANNED,
	MODULE_LOADED,
	MODULE_ERROR,
};

struct imodule_desc {
	enum module_state state;
	char name[STUI3_MODULE_NAME_LEGNTH];
	char *description;
	char path[MODULE_PATH_LEN];
	size_t num_export_names;
	stui3_module_symbol *exports;

	void *dl_handle;
	struct stui3_module_description const *desc;
};

struct isymbol {
	stui3_module_symbol name;
	int owner;
	void *ptr;
};

static struct imodule_desc *module_table;
static size_t num_modules, cap_modules;
static struct isymbol *symbol_table;
static size_t num_symbols, cap_symbols;

static int find_symtab(stui3_module_symbol const name);
static int add_symtab(stui3_module_symbol const name, int const owner, void *const ptr, int const force);
static void symtab_rm_owner(int const owner, int const replace);
static int find_module(char const *const name);
static int is_path_known(char const *const path);
static int scan_module(char const *const filename, int const force, int const close);
static int activate_scanned_module(int const mod, int const force);
static void find_replacement_symbol(int const sym, int const blacklist_module);
static void deactivate_module(int const mod);

int init_symbols() {
	/* no builtin modules for now */

	module_table = malloc(16 * sizeof(struct imodule_desc));
	if(! module_table) return -1;
	symbol_table = malloc(16 * sizeof(struct isymbol));
	if(! symbol_table) {
		free(module_table);
		return -1;
	}

	num_modules = num_symbols = 0;
	cap_modules = cap_symbols = 16;
}

void shutdown_symbols() {
	free(module_table);
	free(symbol_table);
	num_modules = cap_modules = num_symbols = cap_symbols = 0;
}

void *lookup_symbol(stui3_module_symbol const symbol_name) {
	int idx;

	idx = find_symtab(symbol_name);
	if(idx < 0) return NULL;
	return symbol_table[idx].ptr;
}

int lookup_module(char const *const module_name) {
	return find_module(module_name);
}

int load_module(char const *const module_name) {
	char path[MODULE_PATH_LEN];
	int mod;

	snprintf(path, sizeof(path), "%s/%s.so", MODULE_PATH, module_name);
	mod = scan_module(path, 1, 1);
	if(mod < 0) return -1;
	return activate_scanned_module(mod, 1);
}

void unload_module(int module_id) {
	deactivate_module(module_id);
}

size_t symbol_providers(stui3_module_symbol const symbol_name, int *mod_arr, size_t arr_size) {
	size_t i, j, n;

	if(arr_size == 0) return 0;

	n = 0;
	for(i = 0; i < num_modules; ++i) {
		for(j = 0; j < module_table[i].num_export_names; ++j) {
			if(strncmp(module_table[i].exports[j], symbol_name, STUI3_MODULE_SYMBOL_LENGTH) == 0) {
				mod_arr[n++] = i;
				if(n == arr_size) return n;
				break;
			}
		}
	}

	return n;
}

int set_symbol_provider(stui3_module_symbol const symbol_name, int module_id) {
	size_t i;
	int sym;

	sym = find_symtab(symbol_name);
	if(sym < 0) return -1;

	if(module_id < 0 || module_id >= num_modules) {
		goto ret_prev;
	}

rescan_module:
	for(i = 0; i < module_table[module_id].num_export_names; ++i) {
		if(strncmp(symbol_table[sym].name, module_table[module_id].exports[i], STUI3_MODULE_SYMBOL_LENGTH) == 0) {
			if(activate_scanned_module(module_id, 0) < 0) return symbol_table[sym].owner;
			if(strncmp(symbol_table[sym].name, module_table[module_id].exports[i], STUI3_MODULE_SYMBOL_LENGTH) != 0) goto rescan_module;
			symbol_table[sym].owner = module_id;
			symbol_table[sym].ptr = module_table[module_id].desc->pointers[i];
			return module_id;
		}
	}

ret_prev:
	return symbol_table[sym].owner;
}

static int find_symtab(stui3_module_symbol const name) {
	size_t i;

	for(i = 0; i < num_symbols; ++i) {
		if(strncmp(symbol_table[i].name, name, STUI3_MODULE_SYMBOL_LENGTH) == 0) {
			return i;
		}
	}

	return -1;
}

static int add_symtab(stui3_module_symbol const name, int owner, void *const ptr, int force) {
	int new_id;
	struct isymbol *new_symtab;

	if((new_id = find_symtab(name)) >= 0) {
		if(! force) return new_id;
	} else {
		if(num_symbols > INT_MAX) return -1;
		if(num_symbols == cap_symbols) {
			cap_symbols *= 2;
			new_symtab = realloc(symbol_table, cap_symbols * sizeof(struct isymbol));
			if(! new_symtab) {
				cap_symbols /= 2;
				return -1;
			}
			symbol_table = new_symtab;
		}
		new_id = num_symbols++;
	}

	strncpy(symbol_table[new_id].name, name, STUI3_MODULE_SYMBOL_LENGTH - 1);
	symbol_table[new_id].name[STUI3_MODULE_SYMBOL_LENGTH - 1] = '\0';
	symbol_table[new_id].owner = owner;
	symbol_table[new_id].ptr = ptr;

	return new_id;
}

static void symtab_rm_owner(int const owner, int replace) {
	size_t i;

	for(i = 0; i < num_symbols; ++i) {
		if(symbol_table[i].owner == owner) {
			if(replace) {
				find_replacement_symbol(i, owner);
			} else {
				symbol_table[i].owner = -1;
				symbol_table[i].ptr = NULL;
			}
		}
	}
}

static int find_module(char const *const name) {
	size_t i;

	for(i = 0; i < num_modules; ++i) {
		if(strncmp(module_table[i].name, name, STUI3_MODULE_NAME_LEGNTH) == 0) {
			return i;
		}
	}

	return -1;
}

static int is_path_known(char const *const path) {
	size_t i;

	for(i = 0; i < num_modules; ++i) {
		if(strncmp(module_table[i].path, path, MODULE_PATH_LEN) == 0) {
			return i;
		}
	}

	return -1;
}

static int scan_module(char const *const filename, int const force, int const close) {
	int new_module;
	void *dl_handle;
	struct stui3_module_description *description;
	struct imodule_desc *new_modtab;
	int increment = 0;

	if((new_module = is_path_known(filename)) >= 0) {
		if(! force) return new_module;
	} else {
		if(num_modules > INT_MAX) return -1;
		if(num_modules == cap_modules) {
			cap_modules *= 2;
			new_modtab = realloc(module_table, cap_modules * sizeof(struct imodule_desc));
			if(! new_modtab) {
				cap_modules /= 2;
				dlclose(dl_handle);
				return -1;
			}
			module_table = new_modtab;
		}
		new_module = num_modules;
		increment = 1;
	}

	dl_handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
	if(! dl_handle) return -1;

	dlerror();
	description = dlsym(dl_handle, "stui3_module");
	if(! description || dlerror()) {
		dlclose(dl_handle);
		return -1;
	}

	if(description->magic[0] != 0x9A || description->magic[1] != 0x87 ||
		description->magic[2] != 0x6E || description->magic[3] != 0x95) {
		dlclose(dl_handle);
		return -1;
	}
	if(description->minimum_abi > ABI_VERSION || description->deprecation_abi <= ABI_VERSION) {
		dlclose(dl_handle);
		return -1;
	}
	if(strncmp(description->name, filename, STUI3_MODULE_NAME_LEGNTH) != 0) {
		dlclose(dl_handle);
		return -1;
	}

	module_table[new_module].state = MODULE_SCANNED;
	strncpy(module_table[new_module].name, description->name, STUI3_MODULE_NAME_LEGNTH - 1);
	module_table[new_module].name[STUI3_MODULE_NAME_LEGNTH - 1] = '\0';
	module_table[new_module].description = strdup(description->description);
	if(! module_table[new_module].description) {
		dlclose(dl_handle);
		return -1;
	}
	strncpy(module_table[new_module].path, filename, MODULE_PATH_LEN - 1);
	module_table[new_module].path[MODULE_PATH_LEN - 1] = '\0';
	module_table[new_module].num_export_names = description->num_exports;
	module_table[new_module].exports = malloc(description->num_exports * sizeof(stui3_module_symbol));
	if(! module_table[new_module].exports) {
		free(module_table[new_module].description);
		dlclose(dl_handle);
		return -1;
	}
	memcpy(module_table[new_module].exports, description->symbols, description->num_exports * sizeof(stui3_module_symbol));
	if(close) {
		module_table[new_module].dl_handle = NULL;
		module_table[new_module].desc = NULL;
		dlclose(dl_handle);
	} else {
		module_table[new_module].dl_handle = dl_handle;
		module_table[new_module].desc = description;
	}

	if(increment) ++num_modules;
	return new_module;
}

static int activate_scanned_module(int const mod, int const force) {
	size_t i;

	if(mod < 0 || mod >= num_modules) {
		return -1;
	}
	if(module_table[mod].state != MODULE_SCANNED && module_table[mod].state != MODULE_ERROR) {
		return mod;
	}
	if(mod != scan_module(module_table[mod].path, 1, 0)) {
		return -1;
	}

	for(i = 0; i < module_table[mod].desc->num_exports && num_symbols <= INT_MAX; ++i) {
		if(add_symtab(module_table[mod].desc->symbols[i], mod, module_table[mod].desc->pointers[i], force) < 0) {
			/* ??? TODO ??? */
		}
	}

	module_table[mod].state = MODULE_LOADED;
	return mod;
}

static void find_replacement_symbol(int const sym, int const blacklist_module) {
	size_t i, j;

	if(sym < 0 || sym >= num_symbols) {
		return;
	}

	for(i = 0; i < num_modules; ++i) {
		if(i == blacklist_module) continue;
rescan_module:
		for(j = 0; j < module_table[i].num_export_names; ++j) {
			if(strncmp(symbol_table[sym].name, module_table[i].exports[j], STUI3_MODULE_SYMBOL_LENGTH) == 0) {
				if(activate_scanned_module(i, 0) < 0) break;
				if(strncmp(symbol_table[sym].name, module_table[i].exports[j], STUI3_MODULE_SYMBOL_LENGTH) != 0) goto rescan_module;
				symbol_table[sym].owner = i;
				symbol_table[sym].ptr = module_table[i].desc->pointers[j];
				return;
			}
		}
	}

	symbol_table[sym].owner = -1;
	symbol_table[sym].ptr = NULL;
}

static void deactivate_module(int const mod) {
	if(mod < 0 || mod >= num_modules) return;
	if(module_table[mod].state != MODULE_LOADED) return;

	symtab_rm_owner(mod, 1);
	dlclose(module_table[mod].dl_handle);
	module_table[mod].dl_handle = NULL;
	module_table[mod].desc = NULL;
}

