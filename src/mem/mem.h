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

typedef struct mem_data       md_t;
typedef struct mem_data const cmd_t;

uint32_t mem_caps();

stui_result mem_init(char const name[static STUI_NAME_REQUEST_SIZE], uint32_t const flags, md_t *const md, int *const is_parent);

stui_result mem_map(size_t const size, md_t *const md);

stui_result mem_remap(size_t const new_size, md_t *const md);

stui_result mem_unmap(md_t *const md);

static inline void *
mem_derefu(size_t off, cmd_t *const md) {
	switch(md->mode) {
	case MEM_MODE_NONE:
		return NULL;
#if (MEM_BACKEND_SHM)
	case MEM_MODE_SHM:
		return mem_shm_derefu(off, &md->data.shm);
#endif
#if (MEM_BACKEND_LOCAL)
	case MEM_MODE_LOCAL:
		return mem_local_derefu(off, &md->data.local);
#endif
	default:
		return NULL;
	}
}

static inline void *
mem_derefc(size_t off, cmd_t *const md) {
	switch(md->mode) {
	case MEM_MODE_NONE:
		return NULL;
#if (MEM_BACKEND_SHM)
	case MEM_MODE_SHM:
		return mem_shm_derefc(off, &md->data.shm);
#endif
#if (MEM_BACKEND_LOCAL)
	case MEM_MODE_LOCAL:
		return mem_local_derefc(off, &md->data.local);
#endif
	default:
		return NULL;
	}
}

#define DECLARE_DEREFU(TYPE,NAME,OFF,MD) TYPE NAME = *(TYPE *)mem_derefu(OFF,MD);
#define DECLARE_DEREFC(TYPE,NAME,OFF,DEFAULT,MD) TYPE NAME; do { TYPE *_ptr = mem_derefc(OFF,MD); NAME = _ptr ? *_ptr : DEFAULT; } while (0);
#define DECLARE_PTRU(TYPE,NAME,OFF,MD) TYPE *NAME = (TYPE *)mem_derefu(OFF,MD);
#define DECLARE_PTRC(TYPE,NAME,OFF,MD) TYPE *NAME = (TYPE *)mem_derefc(OFF,MD);
#define SET_DEREFU(TYPE,NAME,OFF,MD) NAME = *(TYPE *)mem_derefu(OFF,MD);
#define SET_DEREFC(TYPE,NAME,OFF,MD) if(mem_derefc(OFF,MD)) { NAME = *(TYPE *)mem_derefc(OFF,MD); }
#define SET_PTRU(TYPE,NAME,OFF,MD) NAME = (TYPE *)mem_derefu(OFF,MD);
#define SET_PTRC(TYPE,NAME,OFF,MD) NAME = (TYPE *)mem_derefc(OFF,MD);
#define DEREFU_SET(TYPE,OFF,VALUE,MD) *(TYPE *)mem_derefu(OFF,MD) = (TYPE)VALUE;
#define DEREFC_SET(TYPE,OFF,VALUE,MD) if(mem_derefc(OFF,MD)) { *(TYPE *)mem_derefc(OFF,MD) = (TYPE)VALUE; }

#define MDECLARE_DEREFU(TYPE,NAME,OFF) TYPE NAME = *(TYPE *)mem_derefu(OFF,md);
#define MDECLARE_DEREFC(TYPE,NAME,OFF,DEFAULT) TYPE NAME; do { TYPE *_ptr = mem_derefc(OFF,md); NAME = _ptr ? *_ptr : DEFAULT; } while (0);
#define MDECLARE_PTRU(TYPE,NAME,OFF) TYPE *NAME = (TYPE *)mem_derefu(OFF,md);
#define MDECLARE_PTRC(TYPE,NAME,OFF) TYPE *NAME = (TYPE *)mem_derefc(OFF,md);
#define MSET_DEREFU(TYPE,NAME,OFF) NAME = *(TYPE *)mem_derefu(OFF,md);
#define MSET_DEREFC(TYPE,NAME,OFF) if(mem_derefc(OFF,md)) { NAME = *(TYPE *)mem_derefc(OFF,md); }
#define MSET_PTRU(TYPE,NAME,OFF) NAME = (TYPE *)mem_derefu(OFF,md);
#define MSET_PTRC(TYPE,NAME,OFF) NAME = (TYPE *)mem_derefc(OFF,md);
#define MDEREFU_SET(TYPE,OFF,VALUE) *(TYPE *)mem_derefu(OFF,md) = (TYPE)VALUE;
#define MDEREFC_SET(TYPE,OFF,VALUE) if(mem_derefc(OFF,md)) { *(TYPE *)mem_derefc(OFF,md) = (TYPE)VALUE; }

#endif

