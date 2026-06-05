#ifndef MEM_LOCAL_H
#define MEM_LOCAL_H

#include <stddef.h>

#include "stui.h"

struct mem_local_data {
	void *base;
	size_t mapped_size;
};

stui_result mem_local_init(struct mem_local_data *const mld, int *const is_parent);

stui_result mem_local_map(size_t const size, struct mem_local_data *const mld);

stui_result mem_local_remap(size_t const new_size, struct mem_local_data *const mld);

stui_result mem_local_unmap(struct mem_local_data *const mld);

#endif

