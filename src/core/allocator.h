#ifndef CORE_ALLOCATOR_H
#define CORE_ALLOCATOR_H

#include <stdint.h>

#include "mem/mem.h"

typedef uint64_t allocator_slab_mask
#define ALLOCATOR_SLAB_WIDTH sizeof(allocator_slab_mask)

#define ALLOCATOR_BIG_MAGIC TO_MAGIC_ID("ALLCBIGH")
#define ALLOCATOR_SLAB_MAGIC TO_MAGIC_ID("ALLCSLAB")

struct allocator_big_header {
	magic_id magic;
	size_t onext_header;

	size_t obj_size;
	size_t obj_align;
	size_t o_obj;
};

struct allocator_slab_header {
	magic_id magic;
	size_t onext_header;

	size_t objs_size;
	size_t objs_align;
	size_t pitch;
	allocator_slab_mask free_mask;
	size_t o_objs;

	size_t o_next; /* might be STUI_IHANDLE */
};

struct allocator_shared_data {
	size_t current_size;
	size_t first_header;
	/* some kind of mutex probably */
};

struct allocator_pdata {
	md_t md;
};

stui_result alloc_init(md_t const md, struct allocator_pdata *const ppd);

#endif

