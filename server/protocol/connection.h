#ifndef COMM_CONNECTION_H
#define COMM_CONNECTION_H

#include "protocol.h"
#include "util.h"

struct protocol_connection {
	int client_fd;
	uint8_t hs_buf[MAX(CLIENT_HANDSHAKE_SIZE,SERVER_HANDSHAKE_SIZE)];
	unsigned int hs_rw;
	struct client_handshake c_handshake;
	struct server_handshake s_handshake;
	uint16_t (*htocs)(uint16_t);
	uint16_t (*ctohs)(uint16_t);
	uint32_t (*htocl)(uint32_t);
	uint32_t (*ctohl)(uint32_t);
	uint64_t (*htocll)(uint64_t);
	uint64_t (*ctohll)(uint64_t);
};


void init_connection(int const client_fd, struct protocol_connection *const conn);
/* < 0 means error, 0 means success, > 0 means continue */
int handshake_connection(struct protocol_connection *const conn);

#endif

