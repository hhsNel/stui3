#include "comm.h"

#include "stui3.h"
#include "endian.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

int
send_client_handshake(int const sock_fd) {
	uint8_t buf[CLIENT_HANDSHAKE_SIZE];
	struct client_handshake handshake;
	size_t sent;
	ssize_t r;
	struct pollfd pfd;

	handshake.magic[0] = CLIENT_HANDSHAKE_MAGIC_0;
	handshake.magic[1] = CLIENT_HANDSHAKE_MAGIC_1;
	handshake.magic[2] = CLIENT_HANDSHAKE_MAGIC_2;
	handshake.magic[3] = CLIENT_HANDSHAKE_MAGIC_3;
	handshake.network_order_version = htons(PROTOCOL_VERSION);
	handshake.init_flags = 0;
	if(is_little_endian()) {
		/* little endian */
		handshake.init_flags |= CLIENT_HANDSHAKE_FLAG_LE;
	} else {
		/* big endian */
		handshake.init_flags |= CLIENT_HANDSHAKE_FLAG_BE;
	}

	memcpy(buf+CLIENT_HANDSHAKE_OFF_MAGIC,
		&handshake.magic,
		CLIENT_HANDSHAKE_SZ_MAGIC);
	memcpy(buf+CLIENT_HANDSHAKE_OFF_N_VERSION,
		&handshake.network_order_version,
		CLIENT_HANDSHAKE_SZ_N_VERSION);
	memcpy(buf+CLIENT_HANDSHAKE_OFF_INIT_FLAGS,
		&handshake.init_flags,
		CLIENT_HANDSHAKE_SZ_INIT_FLAGS);

	sent = 0;
	pfd.fd = sock_fd;
	pfd.events = POLLOUT | POLLHUP;
	while(sent < CLIENT_HANDSHAKE_SIZE) {
		r = write(sock_fd, buf + sent, CLIENT_HANDSHAKE_SIZE - sent);
		if(r < 0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				poll(&pfd, 1, -1);
				continue;
			}
			return -STUI3_EUPSTM;
		}
		sent += r;
	}

	return 0;
}

int
recv_server_handshake(int const sock_fd, struct server_handshake *const handshake) {
	uint8_t buf[SERVER_HANDSHAKE_SIZE];
	size_t recvd;
	ssize_t r;
	struct pollfd pfd;
	
	recvd = 0;
	pfd.fd = sock_fd;
	pfd.events = POLLIN;
	while(recvd < SERVER_HANDSHAKE_SIZE) {
		r = read(sock_fd, buf + recvd, SERVER_HANDSHAKE_SIZE - recvd);
		if(r < 0) {
			if(errno == EINTR) continue;
			if(errno == EAGAIN || errno == EWOULDBLOCK) {
				poll(&pfd, 1, -1);
				continue;
			}
			return -STUI3_EUPSTM;
		}
		if(r == 0) {
			return -STUI3_EUPSTM;
		}
		recvd += r;

		if(recvd >= SERVER_HANDSHAKE_OFF_MAGIC + SERVER_HANDSHAKE_SZ_MAGIC) {
			memcpy(&handshake->magic,
				buf+SERVER_HANDSHAKE_OFF_MAGIC,
				SERVER_HANDSHAKE_SZ_MAGIC);
			if(handshake->magic[0] != SERVER_HANDSHAKE_MAGIC_0 ||
				handshake->magic[1] != SERVER_HANDSHAKE_MAGIC_1 ||
				handshake->magic[2] != SERVER_HANDSHAKE_MAGIC_2 ||
				handshake->magic[3] != SERVER_HANDSHAKE_MAGIC_3) {
				return SERVER_HANDSHAKE_REJECTED_MAGIC;
			}
		}

		if(recvd >= SERVER_HANDSHAKE_OFF_N_VERSION + SERVER_HANDSHAKE_SZ_N_VERSION) {
			memcpy(&handshake->network_order_version,
				buf+SERVER_HANDSHAKE_OFF_N_VERSION,
				SERVER_HANDSHAKE_SZ_N_VERSION);
			if(handshake->network_order_version != htons(PROTOCOL_VERSION)) {
				return SERVER_HANDSHAKE_REJECTED_VERSION;
			}
		}
	}

	memcpy(&handshake->rejected,
		buf+SERVER_HANDSHAKE_OFF_REJECTED,
		SERVER_HANDSHAKE_SZ_REJECTED);
	memcpy(&handshake->network_order_caps,
		buf+SERVER_HANDSHAKE_OFF_N_CAPS,
		SERVER_HANDSHAKE_SZ_N_CAPS);

	return handshake->rejected;
}

