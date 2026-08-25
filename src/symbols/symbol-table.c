#include "symbol-table.h"

#include "util.h"

#define MODULE_PATH_LEN MIN(PATH_MAX, 512)
#define MODULE_PATH "."

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <dirent.h>

enum module_state {
	MODULE_BUILTIN,
	MODULE_SCANNED,
	MODULE_LOADED,
	MODULE_ERROR,
};

struct imodule_desc {
	enum module_state state;
	char name[STUI3_MODULE_NAME_LENGTH];
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
static int is_module_used(int const mod);
static int scan_module(char const *const filename, int const force, int const close);
#if !IS_EMPTY(ALL_BUILTIN_MODULES)
static int add_builtin(struct stui3_module_description const *const descr, int const force);
#endif
static int activate_scanned_module(int const mod, int const force);
static void find_replacement_symbol(int const sym, int const blacklist_module);
static void deactivate_module(int const mod, int const find_replacement);

#define DECLARE_BUILTIN_DESCRIPTION(NAME) extern struct stui3_module_description CONCAT(NAME,_stui3_module);
FOREACH_MACRO(DECLARE_BUILTIN_DESCRIPTION, ALL_BUILTIN_MODULES)
#undef DECLARE_BUILTIN_DESCRIPTION

int init_symbols() {
	DIR *d;
	struct dirent *ent;
	char path[MODULE_PATH_LEN];
#if !IS_EMPTY(ALL_BUILTIN_MODULES)
	static struct stui3_module_description *builtin_descriptions[] = {
#define NAME_TO_DECL(NAME) &CONCAT(NAME,_stui3_module),
		FOREACH_MACRO(NAME_TO_DECL, ALL_BUILTIN_MODULES)
#undef NAME_TO_DECL
	};
	unsigned int i;
#endif

	/* no builtin modules for now */

	module_table = malloc(16 * sizeof(struct imodule_desc));
	if(! module_table) return -STUI3_EUPSTM;
	symbol_table = malloc(16 * sizeof(struct isymbol));
	if(! symbol_table) {
		free(module_table);
		return -STUI3_EUPSTM;
	}

	num_modules = num_symbols = 0;
	cap_modules = cap_symbols = 16;

#if !IS_EMPTY(ALL_BUILTIN_MODULES)
	for(i = 0; i < sizeof(builtin_descriptions)/sizeof(*builtin_descriptions); ++i) {
		add_builtin(builtin_descriptions[i], 0);
	}
#endif

	d = opendir(MODULE_PATH);
	if(d) {
		while((ent = readdir(d))) {
			if(strlen(ent->d_name) >= 3 && strcmp(ent->d_name + strlen(ent->d_name) - 3, ".so") == 0) {
				snprintf(path, sizeof(path), "%s/%s", MODULE_PATH, ent->d_name);
				scan_module(path, 0, 1);
			}
		}
	}

	return 0;
}

void shutdown_symbols() {
	size_t i;

	for(i = 0; i < num_modules; ++i) {
		deactivate_module(i, 0);
	}

	free(module_table);
	free(symbol_table);
	num_modules = cap_modules = num_symbols = cap_symbols = 0;
}

void *lookup_symbol(stui3_module_symbol const symbol_name) {
	int idx;

	idx = find_symtab(symbol_name);
	if(idx < 0) {
		if((idx = add_symtab(symbol_name, -1, NULL, 0)) < 0) return NULL;
	}
	if(! symbol_table[idx].ptr) {
		find_replacement_symbol(idx, -1);
	}
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
	if(mod < 0) return mod;
	return activate_scanned_module(mod, 1);
}

void unload_module(int module_id) {
	deactivate_module(module_id, 1);
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
	int old_owner;

	sym = find_symtab(symbol_name);
	if(sym < 0) return sym;

	if(module_id < 0 || (size_t)module_id >= num_modules) {
		goto ret_prev;
	}

rescan_module:
	for(i = 0; i < module_table[module_id].num_export_names; ++i) {
		if(strncmp(symbol_table[sym].name, module_table[module_id].exports[i], STUI3_MODULE_SYMBOL_LENGTH) == 0) {
			if(activate_scanned_module(module_id, 0) < 0) return symbol_table[sym].owner;
			if(strncmp(symbol_table[sym].name, module_table[module_id].exports[i], STUI3_MODULE_SYMBOL_LENGTH) != 0) goto rescan_module;
			old_owner = symbol_table[sym].owner;
			symbol_table[sym].owner = module_id;
			symbol_table[sym].ptr = module_table[module_id].desc->pointers[i];
			if(! is_module_used(old_owner)) {
				deactivate_module(old_owner, 0);
			}
			return module_id;
		}
	}

ret_prev:
	return symbol_table[sym].owner;
}

size_t module_count() {
	return num_modules;
}

void module_info(int const module_id, struct module_info *const arg) {
	size_t export_copy_count;

	if(module_id < 0 || (size_t)module_id >= num_modules) {
		arg->module_id = -1;
		arg->name[0] = '\0';
		return;
	}

	arg->module_id = module_id;
	strncpy(arg->name, module_table[module_id].name, STUI3_MODULE_NAME_LENGTH - 1);
	arg->name[STUI3_MODULE_NAME_LENGTH - 1] = '\0';

	if(arg->description == NULL && arg->description_count == 0) {
		arg->description_count = strlen(module_table[module_id].description);
	} else {
		snprintf(arg->description, arg->description_count, "%s", module_table[module_id].description);
	}

	if(arg->exports == NULL && arg->export_count == 0) {
		arg->export_count = module_table[module_id].num_export_names;
	} else {
		if(arg->export_arg < module_table[module_id].num_export_names) {
			export_copy_count =  module_table[module_id].num_export_names - arg->export_arg;
			if(export_copy_count > arg->export_count) export_copy_count = arg->export_count;
			memcpy(arg->exports, module_table[module_id].exports + arg->export_arg, export_copy_count * sizeof(stui3_module_symbol));
			arg->export_arg = export_copy_count;
		} else {
			arg->export_arg = 0;
		}
	}
}

static int find_symtab(stui3_module_symbol const name) {
	size_t i;

	for(i = 0; i < num_symbols; ++i) {
		if(strncmp(symbol_table[i].name, name, STUI3_MODULE_SYMBOL_LENGTH) == 0) {
			return i;
		}
	}

	return -STUI3_ENOENT;
}

static int add_symtab(stui3_module_symbol const name, int owner, void *const ptr, int force) {
	int new_id;
	struct isymbol *new_symtab;
	int old_owner = -1;

	if((new_id = find_symtab(name)) >= 0) {
		if(! force) return new_id;
		old_owner = symbol_table[new_id].owner;
	} else {
		if(num_symbols > INT_MAX) return -STUI3_ELIMIT;
		if(num_symbols == cap_symbols) {
			cap_symbols *= 2;
			new_symtab = realloc(symbol_table, cap_symbols * sizeof(struct isymbol));
			if(! new_symtab) {
				cap_symbols /= 2;
				return -STUI3_EUPSTM;
			}
			symbol_table = new_symtab;
		}
		new_id = num_symbols++;
		strncpy(symbol_table[new_id].name, name, STUI3_MODULE_SYMBOL_LENGTH - 1);
		symbol_table[new_id].name[STUI3_MODULE_SYMBOL_LENGTH - 1] = '\0';
	}

	symbol_table[new_id].owner = owner;
	symbol_table[new_id].ptr = ptr;
	if(old_owner != -1) {
		if(! is_module_used(old_owner)) {
			deactivate_module(old_owner, 0);
		}
	}

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
		if(strncmp(module_table[i].name, name, STUI3_MODULE_NAME_LENGTH) == 0) {
			return i;
		}
	}

	return -STUI3_ENOENT;
}

static int is_path_known(char const *const path) {
	size_t i;

	for(i = 0; i < num_modules; ++i) {
		if(strncmp(module_table[i].path, path, MODULE_PATH_LEN) == 0) {
			return i;
		}
	}

	return -STUI3_ENOENT;
}

static int is_module_used(int const mod) {
	size_t i;

	for(i = 0; i < num_symbols; ++i) {
		if(symbol_table[i].owner == mod) return 1;
	}

	return 0;
}

static int scan_module(char const *const filename, int const force, int const close) {
	int new_module;
	void *dl_handle;
	struct stui3_module_description *description;
	struct imodule_desc *new_modtab;
	int increment = 0;

	if((new_module = is_path_known(filename)) >= 0) {
		if(! force) return new_module;
		deactivate_module(new_module, 1);
		free(module_table[new_module].description);
		free(module_table[new_module].exports);
		module_table[new_module].description = NULL;
		module_table[new_module].exports = NULL;
		module_table[new_module].state = MODULE_ERROR;
	} else {
		if(num_modules > INT_MAX) return -STUI3_ELIMIT;
		if(num_modules == cap_modules) {
			cap_modules *= 2;
			new_modtab = realloc(module_table, cap_modules * sizeof(struct imodule_desc));
			if(! new_modtab) {
				cap_modules /= 2;
				return -STUI3_EUPSTM;
			}
			module_table = new_modtab;
		}
		new_module = num_modules;
		increment = 1;
	}

	dl_handle = dlopen(filename, RTLD_NOW | RTLD_LOCAL);
	if(! dl_handle) return -STUI3_EUPSTM;

	dlerror();
	description = dlsym(dl_handle, "stui3_module");
	if(! description || dlerror()) {
		dlclose(dl_handle);
		return -STUI3_EIDATA;
	}

	if(description->magic[0] != 0x9A || description->magic[1] != 0x87 ||
		description->magic[2] != 0x6E || description->magic[3] != 0x95) {
		dlclose(dl_handle);
		return -STUI3_ECHECK;
	}
	if(description->minimum_abi > ABI_VERSION || description->deprecation_abi <= ABI_VERSION) {
		dlclose(dl_handle);
		return -STUI3_EIACTN;
	}

	strncpy(module_table[new_module].name, description->name, STUI3_MODULE_NAME_LENGTH - 1);
	module_table[new_module].name[STUI3_MODULE_NAME_LENGTH - 1] = '\0';
	if(description->description) module_table[new_module].description = strdup(description->description);
	else module_table[new_module].description = strdup("");
	if(! module_table[new_module].description) {
		dlclose(dl_handle);
		return -STUI3_EUPSTM;
	}
	strncpy(module_table[new_module].path, filename, MODULE_PATH_LEN - 1);
	module_table[new_module].path[MODULE_PATH_LEN - 1] = '\0';
	module_table[new_module].num_export_names = description->num_exports;
	module_table[new_module].exports = malloc(description->num_exports * sizeof(stui3_module_symbol));
	if(! module_table[new_module].exports) {
		free(module_table[new_module].description);
		dlclose(dl_handle);
		return -STUI3_EUPSTM;
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
	module_table[new_module].state = MODULE_SCANNED;

	if(increment) ++num_modules;
	return new_module;
}

#if !IS_EMPTY(ALL_BUILTIN_MODULES)
static int add_builtin(struct stui3_module_description const *const descr, int const force) {
	int new_module;
	struct imodule_desc *new_modtab;
	int increment = 0;
	size_t i;

	if((new_module = find_module(descr->name)) >= 0) {
		if(! force) return new_module;
		if(module_table[new_module].state == MODULE_BUILTIN) return new_module;
		deactivate_module(new_module, 1);
		free(module_table[new_module].description);
		free(module_table[new_module].exports);
		module_table[new_module].description = NULL;
		module_table[new_module].exports = NULL;
		module_table[new_module].state = MODULE_ERROR;
	} else {
		if(num_modules > INT_MAX) return -STUI3_ELIMIT;
		if(num_modules == cap_modules) {
			cap_modules *= 2;
			new_modtab = realloc(module_table, cap_modules * sizeof(struct imodule_desc));
			if(! new_modtab) {
				cap_modules /= 2;
				return -STUI3_EUPSTM;
			}
			module_table = new_modtab;
		}
		new_module = num_modules;
		increment = 1;
	}

	if(descr->magic[0] != 0x9A || descr->magic[1] != 0x87 ||
		descr->magic[2] != 0x6E || descr->magic[3] != 0x95) {
		return -STUI3_ECHECK;
	}
	if(descr->minimum_abi > ABI_VERSION || descr->deprecation_abi <= ABI_VERSION) {
		return -STUI3_EIACTN;
	}

	strncpy(module_table[new_module].name, descr->name, STUI3_MODULE_NAME_LENGTH - 1);
	module_table[new_module].name[STUI3_MODULE_NAME_LENGTH - 1] = '\0';
	if(descr->description) module_table[new_module].description = strdup(descr->description);
	else module_table[new_module].description = strdup("");
	if(! module_table[new_module].description) {
		return -STUI3_EUPSTM;
	}
	module_table[new_module].path[0] = '\0';
	module_table[new_module].num_export_names = descr->num_exports;
	module_table[new_module].exports = malloc(descr->num_exports * sizeof(stui3_module_symbol));
	if(! module_table[new_module].exports) {
		free(module_table[new_module].description);
		return -STUI3_EUPSTM;
	}
	memcpy(module_table[new_module].exports, descr->symbols, descr->num_exports * sizeof(stui3_module_symbol));
	module_table[new_module].dl_handle = NULL;
	module_table[new_module].desc = descr;
	module_table[new_module].state = MODULE_BUILTIN;

	for(i = 0; i < module_table[new_module].desc->num_exports && num_symbols <= INT_MAX; ++i) {
		if(add_symtab(module_table[new_module].desc->symbols[i], new_module, module_table[new_module].desc->pointers[i], force) < 0) {
			continue;
		}
	}
	module_table[new_module].desc->load_hook();

	if(increment) ++num_modules;
	return new_module;
}
#endif

static int activate_scanned_module(int const mod, int const force) {
	size_t i;

	if(mod < 0 || (size_t)mod >= num_modules) {
		return -STUI3_ENOENT;
	}
	if(module_table[mod].state != MODULE_SCANNED && module_table[mod].state != MODULE_ERROR) {
		return mod;
	}
	if(mod != scan_module(module_table[mod].path, 1, 0)) {
		return -STUI3_ENOENT;
	}

	for(i = 0; i < module_table[mod].desc->num_exports && num_symbols <= INT_MAX; ++i) {
		if(add_symtab(module_table[mod].desc->symbols[i], mod, module_table[mod].desc->pointers[i], force) < 0) {
			continue;
		}
	}

	module_table[mod].state = MODULE_LOADED;
	module_table[mod].desc->load_hook();
	return mod;
}

static void find_replacement_symbol(int const sym, int const blacklist_module) {
	size_t i, j;

	if(sym < 0 || (size_t)sym >= num_symbols) {
		return;
	}

	for(i = 0; i < num_modules; ++i) {
		if(i == (size_t)blacklist_module) continue;
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

static void deactivate_module(int const mod, int const find_replacement) {
	if(mod < 0 || (size_t)mod >= num_modules) return;
	if(module_table[mod].state != MODULE_LOADED) return;

	symtab_rm_owner(mod, find_replacement);
	module_table[mod].desc->unload_hook();
	dlclose(module_table[mod].dl_handle);
	module_table[mod].dl_handle = NULL;
	module_table[mod].desc = NULL;
	module_table[mod].state = MODULE_SCANNED;
}

