#ifndef MEM_SHM_H
#define MEM_SHM_H

#include <stddef.h>

#include "stui.h"

#define SHM_PREFIX "/stui-"

struct mem_shm_data {
	char fname[STUI_NAME_REQUEST_SIZE + sizeof(SHM_PREFIX)];
	int fd;
	void *base;
	size_t mapped_size;
};

stui_result mem_shm_init(char const name[static STUI_NAME_REQUEST_SIZE], struct mem_shm_data *const msd, int *const is_parent);

stui_result mem_shm_map(size_t const size, int is_parent, struct mem_shm_data *const msd);

stui_result mem_shm_remap(size_t const new_size, struct mem_shm_data *const msd);

stui_result mem_shm_unmap(int is_parent, struct mem_shm_data *const msd);

#endif

