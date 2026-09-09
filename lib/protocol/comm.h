#ifndef PROTOCOL_COMM_H
#define PROTOCOL_COMM_H

#include "protocol.h"

struct client_protocol {
	struct server_handshake s_hs;
	int sock_fd;

	struct message_header own_headers[128];
	uint8_t own_bodies[128][MSG_PAYLOAD_MAX_LENGTH];
	struct message_header server_headers[256];
	uint8_t server_bodies[256][MSG_PAYLOAD_MAX_LENGTH];
	uint8_t own_seqno;
	uint8_t own_acked_seqno;
	uint8_t server_expected_seqno;
	uint8_t server_acked_seqno;
	uint8_t server_processed_seqno;
	uint32_t server_future_seqnos[4];
};

/* < 0 means error, 0 means success, > 0 means incompatible */
int init_client_protocol(int const sock_fd, struct client_protocol *const cp);

/* < 0 means error, 0 means success, > 0 means wait */
int send_msg(struct client_protocol *const cp, struct message_header const head, uint8_t const *const data);
/* < 0 means error, 0 means success, > 0 means nothing to read */
int recv_msg(struct client_protocol *const cp, struct message_header *const head, uint8_t *const data);
/* < 0 means error, 0 means success */
int run_client_protocol(struct client_protocol *const cp);

#endif

