#ifndef MEM_LOCAL_H
#define MEM_LOCAL_H

#include <stddef.h>

#include "stui.h"
#include "stui-errno.h"

struct mem_local_data {
	void *base;
	size_t mapped_size;
};

stui_result mem_local_init(struct mem_local_data *const mld, int *const is_parent);

stui_result mem_local_map(size_t const size, struct mem_local_data *const mld);

stui_result mem_local_remap(size_t const new_size, struct mem_local_data *const mld);

stui_result mem_local_unmap(struct mem_local_data *const mld);

static inline void *
mem_local_derefu(size_t off, struct mem_local_data const *const mld) {
	return (char *)mld->base + off;
}

static inline void *
mem_local_derefc(size_t off, struct mem_local_data const *const mld) {
	if(off >= mld->mapped_size) {
		stui_errno = STUI_EINVAL;
		return NULL;
	}
	return mem_local_derefu(off, mld);
}

#endif

