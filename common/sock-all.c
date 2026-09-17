#include "sock-all.h"

#include "stui3.h"

#include <unistd.h>
#include <poll.h>
#include <errno.h>
#include <sys/socket.h>

int write_all(int fd, void const *buf, size_t sz) {
	ssize_t ret;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLOUT | POLLHUP;
	while(sz > 0) {
		if((ret = write(fd, buf, sz)) < 0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				poll(&pfd, 1, -1);
				continue;
			}
			else return -STUI3_EUPSTM;
		}
		sz -= ret;
		buf = (char *)buf + ret;
	}

	return 0;
}

int read_expected(int fd, void *buf, size_t sz) {
	ssize_t ret;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN | POLLHUP;
	while(sz > 0) {
		if((ret = read(fd, buf, sz)) < 0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				poll(&pfd, 1, -1);
				continue;
			}
			else return -STUI3_EUPSTM;
		}
		sz -= ret;
		buf = (char *)buf + ret;
	}

	return 0;
}

int peek_expected(int fd, void *buf, size_t sz) {
	ssize_t ret;
	struct pollfd pfd;

	pfd.fd = fd;
	pfd.events = POLLIN | POLLHUP;
	while(sz > 0) {
		if((ret = recv(fd, buf, sz, MSG_PEEK)) < 0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				poll(&pfd, 1, -1);
				continue;
			}
			else return -STUI3_EUPSTM;
		}
		sz -= ret;
		buf = (char *)buf + ret;
	}

	return 0;
}

