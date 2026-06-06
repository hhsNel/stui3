#include "shm.h"

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

stui_result
mem_shm_init(char const name[static STUI_NAME_REQUEST_SIZE], struct mem_shm_data *const msd, int *const is_parent)
{
	unsigned int i;
	int fd;

	/* in case of an error */
	msd->fd = -1;

	memcpy(msd->fname, SHM_PREFIX, sizeof(SHM_PREFIX) - 1);

	for(i = 0; i < STUI_NAME_REQUEST_SIZE; ++i) {
		if(!name[i]) break;

		if( (name[i] >= 'a' && name[i] <= 'z') ||
			(name[i] >= 'A' && name[i] <= 'Z') ||
			(name[i] >= '0' && name[i] <= '9') ) {
			msd->fname[sizeof(SHM_PREFIX) - 1 + i] = name[i];
		} else {
			msd->fname[sizeof(SHM_PREFIX) - 1 + i] = '-';
		}
	}

	msd->fname[sizeof(SHM_PREFIX) - 1 + i] = '\0';

	fd = shm_open(msd->fname, O_RDWR, S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH /* 0666*/);
	if(fd >= 0) {
		*is_parent = 0;
	} else {
		if(errno != ENOENT) {
			return STUI_EUPERR;
		}

		fd = shm_open(msd->fname, O_CREAT | O_EXCL | O_RDWR, S_IRUSR|S_IWUSR | S_IRGRP|S_IWGRP | S_IROTH|S_IWOTH /* 0666*/);
		if(fd < 0) {
			return STUI_EUPERR;
		}
		*is_parent = 1;
	}

	msd->fd = fd;
	msd->base = NULL;
	msd->mapped_size = 0;

	return STUI_OK;
}

stui_result
mem_shm_map(size_t const size, int is_parent, struct mem_shm_data *const msd)
{
	void *ptr;

	if(msd->fd < 0 || msd->base || msd->mapped_size) {
		return STUI_EOORDR;
	}

	if(is_parent) {
		if(ftruncate(msd->fd, size) < 0) {
			return STUI_EUPERR;
		}
	}

	ptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, msd->fd, 0);
	if(ptr == MAP_FAILED) {
		return STUI_EUPERR;
	}

	msd->base = ptr;
	msd->mapped_size = size;

	return STUI_OK;
}

stui_result
mem_shm_remap(size_t const new_size, struct mem_shm_data *const msd)
{
	void *new_ptr;

	if(msd->fd < 0 || !msd->base || !msd->mapped_size) {
		return STUI_EOORDR;
	}

	if(ftruncate(msd->fd, new_size) < 0) {
		return STUI_EUPERR;
	}

	new_ptr = mremap(msd->base, msd->mapped_size, new_size, MREMAP_MAYMOVE);
	if(new_ptr == MAP_FAILED) {
		return STUI_EUPERR;
	}

	msd->base = new_ptr;
	msd->mapped_size = new_size;

	return STUI_OK;
}

stui_result
mem_shm_unmap(int is_parent, struct mem_shm_data *const msd)
{
	stui_result res;

	res = STUI_OK;

	if(msd->fd < 0 || !msd->base || !msd->mapped_size) {
		res = STUI_EOORDR;
	}

	if(msd->base && munmap(msd->base, msd->mapped_size) < 0) {
		res = STUI_EUPERR;
	}

	if(msd->fd >= 0 && close(msd->fd) < 0) {
		res = STUI_EUPERR;
	}

	if(is_parent) {
		if(msd->fd >= 0 && shm_unlink(msd->fname) < 0) {
			res = STUI_EUPERR;
		}
	}

	msd->fd = -1;
	msd->base = NULL;
	msd->mapped_size = 0;

	return res;
}

