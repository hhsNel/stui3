#include "mem.h"

uint32_t
mem_caps()
{

	return 0
#if (MEM_BACKEND_SHM)
		| STUI_MEM_SHM
#endif
#if (MEM_BACKEND_LOCAL)
		| STUI_MEM_LOCAL
#endif
		;
}

stui_result
mem_init(char const name[static STUI_NAME_REQUEST_SIZE], uint32_t const flags, struct mem_data *const md, int *const is_parent)
{
	stui_result res;

	md->mode = MEM_MODE_NONE;

	if(0) {}

#if (MEM_BACKEND_SHM)
	else if(flags & STUI_MEM_SHM) {
		res = mem_shm_init(name, &md->data.shm, &md->is_parent);
		if(STUI_IS_OK(res)) {
			md->mode = MEM_MODE_SHM;
		}
		*is_parent = md->is_parent;
		return res;
	}
#endif
#if (MEM_BACKEND_LOCAL)
	else if(flags & STUI_MEM_LOCAL) {
		res = mem_local_init(&md->data.local, &md->is_parent);
		if(STUI_IS_OK(res)) {
			md->mode = MEM_MODE_LOCAL;
		}
		*is_parent = md->is_parent;
		return res;
	}
#endif

	/* no backend matched */
	return STUI_ENSPRT;
}

stui_result
mem_map(size_t const size, struct mem_data *const md)
{
	if(size == 0) {
		return STUI_EINVAL;
	}

	switch(md->mode) {
	case MEM_MODE_NONE:
		return STUI_EOORDR;
#if (MEM_BACKEND_SHM)
	case MEM_MODE_SHM:
		return mem_shm_map(size, md->is_parent, &md->data.shm);
#endif
#if (MEM_BACKEND_LOCAL)
	case MEM_MODE_LOCAL:
		return mem_local_map(size, &md->data.local);
#endif
	default:
		/* no backend matched */
		return STUI_ECRPTD;
	}
}

stui_result
mem_remap(size_t const new_size, struct mem_data *const md)
{
	if(new_size == 0) {
		return STUI_EINVAL;
	}

	switch(md->mode) {
	case MEM_MODE_NONE:
		return STUI_EOORDR;
#if (MEM_BACKEND_SHM)
	case MEM_MODE_SHM:
		return mem_shm_remap(new_size, &md->data.shm);
#endif
#if (MEM_BACKEND_LOCAL)
	case MEM_MODE_LOCAL:
		return mem_local_remap(new_size, &md->data.local);
#endif
	default:
		/* no backend matched */
		return STUI_ECRPTD;
	}
}

stui_result
mem_unmap(struct mem_data *const md)
{
	switch(md->mode) {
	case MEM_MODE_NONE:
		return STUI_EOORDR;
#if (MEM_BACKEND_SHM)
	case MEM_MODE_SHM:
		md->mode = MEM_MODE_NONE;
		return mem_shm_unmap(md->is_parent, &md->data.shm);
#endif
#if (MEM_BACKEND_LOCAL)
	case MEM_MODE_LOCAL:
		md->mode = MEM_MODE_NONE;
		return mem_local_unmap(&md->data.local);
#endif
	default:
		/* no backend matched */
		return STUI_ECRPTD;
	}
}

