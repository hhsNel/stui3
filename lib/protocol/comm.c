#include "comm.h"

#include "stui3.h"
#include "endian.h"
#include "crc8.h"
#include "sock-all.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define MIN(A,B) (((A) < (B)) ? (A) : (B))

static int send_client_handshake(int const sock_fd);
static int recv_server_handshake(int const sock_fd, struct server_handshake *const handshake);
static int serialize_header(int const sock_fd, struct message_header *const head);
/* < 0 means error, 0 means success, > 0 means corrupted */
static int deserialize_header(int const sock_fd, struct message_header *const head);

int
init_client_protocol(int const sock_fd, struct client_protocol *const cp) {
	int r;

	if((r = send_client_handshake(sock_fd)) < 0) return r;
	if((r = recv_server_handshake(sock_fd, &cp->s_hs)) != 0) return r;

	cp->sock_fd = sock_fd;
	cp->own_seqno = 0;
	cp->own_acked_seqno = -1;
	cp->server_expected_seqno = 0;
	cp->server_acked_seqno = -1;
	cp->server_processed_seqno = -1;
	memset(&cp->server_future_seqnos, 0, sizeof(cp->server_future_seqnos));

	return 0;
}

int
send_msg(struct client_protocol *const cp, struct message_header const head, uint8_t const *const data) {
	if(cp->own_acked_seqno == (uint8_t)(cp->own_seqno - 128)) {
		return STUI3_ELIMIT;
	}
	if(head.payload_sz > MSG_PAYLOAD_MAX_LENGTH) {
		return -STUI3_EIACTN;
	}

	cp->own_headers[cp->own_seqno % 128] = head;
	cp->own_headers[cp->own_seqno % 128].magic[0] = MESSAGE_HEADER_MAGIC_0;
	cp->own_headers[cp->own_seqno % 128].magic[1] = MESSAGE_HEADER_MAGIC_1;
	cp->own_headers[cp->own_seqno % 128].magic[2] = MESSAGE_HEADER_MAGIC_2;
	cp->own_headers[cp->own_seqno % 128].magic[3] = MESSAGE_HEADER_MAGIC_3;
	cp->own_headers[cp->own_seqno % 128].seqno = cp->own_seqno;
	memcpy(cp->own_bodies[cp->own_seqno % 128], data, head.payload_sz);
	++cp->own_seqno;

	return 0;
}

int
recv_msg(struct client_protocol *const cp, struct message_header *const head, uint8_t *const data) {
	if((uint8_t)(cp->server_processed_seqno + 1) == cp->server_expected_seqno) {
		/* TODO: scan for MSG_FLAG_INDEPENDENT messages */
		return 1;
	}

	++cp->server_processed_seqno;
	memcpy(head, &cp->server_headers[cp->server_processed_seqno], sizeof(struct message_header));
	memcpy(data, cp->server_bodies[cp->server_processed_seqno], head->payload_sz);

	return 0;
}

int
run_client_protocol(struct client_protocol *const cp) {
	struct pollfd pfd;
	int r;
	struct message_header head;
	uint8_t own_sent_seqno;
	uint8_t crc8;
	uint8_t diff;
	uint8_t resync_buf[4];
	uint8_t ignore;

	own_sent_seqno = cp->own_acked_seqno;

	pfd.fd = cp->sock_fd;
	pfd.events = 0;
	if(cp->own_seqno != own_sent_seqno) {
		pfd.events |= POLLOUT;
	}
	if(cp->server_processed_seqno + 1 != cp->server_expected_seqno) {
		pfd.events |= POLLIN;
	}

	while((r = poll(&pfd, 1, -1)) > 0 || errno == EAGAIN || errno == EINTR) {
		if(pfd.revents & POLLOUT) {
			cp->own_headers[own_sent_seqno%128].seq_ack = cp->server_expected_seqno;
			if((r = serialize_header(cp->sock_fd, &cp->own_headers[own_sent_seqno%128])) < 0) {
				return r;
			}
			r = write_all(cp->sock_fd,
				cp->own_bodies[own_sent_seqno%128],
				cp->own_headers[own_sent_seqno%128].payload_sz);
			if(r < 0) return r;
			++own_sent_seqno;
			if(own_sent_seqno == cp->own_seqno) {
				pfd.events &= ~POLLOUT;
			}
		}
		if(pfd.revents & POLLIN) {
			if((r = deserialize_header(cp->sock_fd, &head)) < 0) {
				return r;
			}
			if(r > 0) {
			resync:
				while((r = peek_expected(cp->sock_fd, resync_buf, 4)) >= 0) {
					if(resync_buf[0] == MESSAGE_HEADER_MAGIC_0 &&
						resync_buf[1] == MESSAGE_HEADER_MAGIC_1 &&
						resync_buf[2] == MESSAGE_HEADER_MAGIC_2 &&
						resync_buf[3] == MESSAGE_HEADER_MAGIC_3) {
						break;
					}
					if((r = read_expected(cp->sock_fd, &ignore, 1)) < 0) return r;
				}
				if(r < 0) return r;
				continue;
			}

			r = read_expected(cp->sock_fd,
				cp->server_bodies[head.seqno],
				head.payload_sz);
			if(r < 0) return r;

			if(head.msg_flags & MSG_FLAG_PAYLOAD_CRC8) {
				if((r = read_expected(cp->sock_fd, &crc8, 1)) < 0) return r;
				if(crc8 != protocol_crc8(cp->server_bodies[head.seqno], head.payload_sz)) {
					goto resync;
				}
			}

			cp->server_headers[head.seqno] = head;

			cp->own_acked_seqno = head.seq_ack - 1;

			diff = head.seqno - cp->server_expected_seqno;
			if(diff < 128) {
				if(diff < 64) cp->server_future_seqnos[0] |= 1ULL << diff;
				else cp->server_future_seqnos[1] |= 1ULL << (diff - 64);
			}
			while(cp->server_future_seqnos[0] & 1) {
				++cp->server_expected_seqno;
				cp->server_future_seqnos[0] >>= 1;
				if(cp->server_future_seqnos[1] & 1) {
					cp->server_future_seqnos[0] |= 0x8000000000000000;
				}
				cp->server_future_seqnos[1] >>= 1;
			}
		}

		if(! pfd.events) break;
	}

	return 0;
}

static int
send_client_handshake(int const sock_fd) {
	uint8_t buf[CLIENT_HANDSHAKE_SIZE];
	struct client_handshake handshake;
	ssize_t r;

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

	if((r = write_all(sock_fd, buf, sizeof(buf))) < 0) return r;

	return 0;
}

static int
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

static int
serialize_header(int const sock_fd, struct message_header *const head) {
	uint8_t buf[MESSAGE_HEADER_SIZE];

	memcpy(buf+MESSAGE_HEADER_OFF_MAGIC,
		&head->magic,
		MESSAGE_HEADER_SZ_MAGIC);
	memcpy(buf+MESSAGE_HEADER_OFF_FLAGS,
		&head->msg_flags,
		MESSAGE_HEADER_SZ_FLAGS);
	memcpy(buf+MESSAGE_HEADER_OFF_PLD_SZ,
		&head->payload_sz,
		MESSAGE_HEADER_SZ_PLD_SZ);
	memcpy(buf+MESSAGE_HEADER_OFF_PLD_TYPE,
		&head->payload_type,
		MESSAGE_HEADER_SZ_PLD_TYPE);
	memcpy(buf+MESSAGE_HEADER_OFF_SEQNO,
		&head->seqno,
		MESSAGE_HEADER_SZ_SEQNO);
	memcpy(buf+MESSAGE_HEADER_OFF_SEQACK,
		&head->seq_ack,
		MESSAGE_HEADER_SZ_SEQACK);

	head->crc8 = protocol_crc8(buf, MESSAGE_HEADER_OFF_CRC);

	memcpy(buf+MESSAGE_HEADER_OFF_CRC,
		&head->crc8,
		MESSAGE_HEADER_SZ_CRC);

	return write_all(sock_fd, buf, sizeof(buf));
}

static int
deserialize_header(int const sock_fd, struct message_header *const head) {
	uint8_t buf[MESSAGE_HEADER_SIZE];
	int r;

	if((r = read_expected(sock_fd, buf, MESSAGE_HEADER_SZ_MAGIC)) < 0) return r;

	memcpy(&head->magic,
			buf+MESSAGE_HEADER_OFF_MAGIC,
			MESSAGE_HEADER_SZ_MAGIC);

	if(head->magic[0] != MESSAGE_HEADER_MAGIC_0 ||
		head->magic[1] != MESSAGE_HEADER_MAGIC_1 ||
		head->magic[2] != MESSAGE_HEADER_MAGIC_2 ||
		head->magic[3] != MESSAGE_HEADER_MAGIC_3) {
		return 1;
	}

	if((r = read_expected(sock_fd,
			buf + MESSAGE_HEADER_SZ_MAGIC,
			sizeof(buf) - MESSAGE_HEADER_SZ_MAGIC)) < 0) return r;

	memcpy(&head->msg_flags,
			buf+MESSAGE_HEADER_OFF_FLAGS,
			MESSAGE_HEADER_SZ_FLAGS);
	memcpy(&head->payload_sz,
			buf+MESSAGE_HEADER_OFF_PLD_SZ,
			MESSAGE_HEADER_SZ_PLD_SZ);
	memcpy(&head->payload_type,
			buf+MESSAGE_HEADER_OFF_PLD_TYPE,
			MESSAGE_HEADER_SZ_PLD_TYPE);
	memcpy(&head->seqno,
			buf+MESSAGE_HEADER_OFF_SEQNO,
			MESSAGE_HEADER_SZ_SEQNO);
	memcpy(&head->seq_ack,
			buf+MESSAGE_HEADER_OFF_SEQACK,
			MESSAGE_HEADER_SZ_SEQACK);
	memcpy(&head->crc8,
			buf+MESSAGE_HEADER_OFF_CRC,
			MESSAGE_HEADER_SZ_CRC);

	if(head->crc8 != protocol_crc8(buf, MESSAGE_HEADER_OFF_CRC)) {
		return 1;
	}

	return 0;
}

