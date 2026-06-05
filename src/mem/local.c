#include "local.h"

#include <sys/mman.h>

stui_result
mem_local_init(struct mem_local_data *const mld, int *const is_parent)
{
	mld->base = NULL;
	mld->mapped_size = 0;

	*is_parent = 1;

	return STUI_OK;
}

stui_result
mem_local_map(size_t const size, struct mem_local_data *const mld)
{
	void *ptr;

	if(mld->base || mld->mapped_size) {
		return STUI_EOORDR;
	}

	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if(ptr == MAP_FAILED) {
		return STUI_EUPERR;
	}

	mld->base = ptr;
	mld->mapped_size = size;

	return STUI_OK;
}

stui_result
mem_local_remap(size_t const new_size, struct mem_local_data *const mld)
{
	void *new_ptr;

	if(!mld->base || !mld->mapped_size) {
		return STUI_EOORDR;
	}

	new_ptr = mremap(mld->base, mld->mapped_size, new_size, MREMAP_MAYMOVE);
	if(new_ptr == MAP_FAILED) {
		return STUI_EUPERR;
	}

	mld->base = new_ptr;
	mld->mapped_size = new_size;

	return STUI_OK;
}

stui_result
mem_local_unmap(struct mem_local_data *const mld)
{
	stui_result res;

	res = STUI_OK;

	if(!mld->base || !mld->mapped_size) {
		return STUI_EOORDR;
	}

	if(munmap(mld->base, mld->mapped_size) < 0) {
		res = STUI_EUPERR;
	}

	mld->base = NULL;
	mld->mapped_size = 0;

	return res;
}

