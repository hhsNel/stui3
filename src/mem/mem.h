#ifndef MEM_MEM_H
#define MEM_MEM_H

#include "stui.h"
#include "util.h"

#ifndef MEM_BACKEND_SHM
#define MEM_BACKEND_SHM 0
#endif
#ifndef MEM_BACKEND_LOCAL
#define MEM_BACKEND_LOCAL 0
#endif
#if (MEM_BACKEND_SHM) == 0 && (MEM_BACKEND_LOCAL) == 0
#error "no mem backend included"
#endif
#if (MEM_BACKEND_SHM)
MESSAGE("including mem/shm.h")
#include "shm.h"
#endif
#if (MEM_BACKEND_LOCAL)
MESSAGE("including mem/local.h")
#include "local.h"
#endif

enum mem_mode {
	MEM_MODE_NONE = 0,
#if (MEM_BACKEND_SHM)
	MEM_MODE_SHM,
#endif
#if (MEM_BACKEND_LOCAL)
	MEM_MODE_LOCAL,
#endif
};

struct mem_data {
	enum mem_mode mode;
	union {
#if (MEM_BACKEND_SHM)
		struct mem_shm_data shm;
#endif
#if (MEM_BACKEND_LOCAL)
		struct mem_local_data local;
#endif
	} data;
	int is_parent;
};

uint32_t mem_caps();

stui_result mem_init(char const name[static STUI_NAME_REQUEST_SIZE], uint32_t const flags, struct mem_data *const md, int *const is_parent);

stui_result mem_map(size_t const size, struct mem_data *const md);

stui_result mem_remap(size_t const new_size, struct mem_data *const md);

stui_result mem_unmap(struct mem_data *const md);

#endif

