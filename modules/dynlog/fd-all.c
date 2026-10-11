#include "fd-all.h"

#include <unistd.h>
#include <errno.h>

int
STUI3_MODULE_NAME(write_all)(int fd, void const *buf, size_t sz) {
	int ret;
	unsigned int retries;

	retries = 8;
	while(sz > 0) {
		if((ret = write(fd, buf, sz)) < 0 && errno == EINTR) {
			if(retries--) continue;
			else return -1;
		}
		if(ret == 0) return -1;
		sz -= ret;
		buf = (char *)buf + ret;
	}

	return 0;
}

int
STUI3_MODULE_NAME(read_expected)(int fd, void *buf, size_t sz) {
	int ret;
	unsigned int retries;

	retries = 8;
	while(sz > 0) {
		if((ret = read(fd, buf, sz)) < 0 && errno == EINTR) {
			if(retries--) continue;
			else return -1;
		}
		if(ret == 0) return -1;
		sz -= ret;
		buf = (char *)buf + ret;
	}

	return 0;
}

