#ifndef COMMON_TRANSPORT_COMM_H
#define COMMON_TRANSPORT_COMM_H

#include "protocol.h"
#include "util.h"

#include <stddef.h>

#define TP_FLAG_SEND_ACK 0x80
#define TP_FLAG_NO_READ 0x40
#define TP_FLAG_SWAP_ENDIANNESS 0x01

enum transport_protocol_state {
	TP_STATE_IDLE,
	TP_STATE_WRITING_HEADER,
	TP_STATE_WRITING_BODY,
	TP_STATE_READING_HEADER,
	TP_STATE_READING_BODY,
	TP_STATE_WRITING_LP,
	TP_STATE_READING_LP,
	TP_STATE_RESYNC,
	TP_STATE_IGNORE,
};

struct transport_protocol {
	int sock_fd;
	uint8_t flags;

	enum transport_protocol_state state;
	uint8_t w_head_buf[MAX(MESSAGE_HEADER_SIZE,LP_HEADER_SIZE)];
	uint8_t w_item_id;
	size_t progress;

	struct message_header own_headers[128];
	uint8_t own_bodies[128][MSG_PAYLOAD_MAX_LENGTH];
	struct message_header other_headers[256];
	uint8_t other_bodies[256][MSG_PAYLOAD_MAX_LENGTH];
	uint8_t own_seqno;
	uint8_t own_sent_seqno;
	uint8_t own_acked_seqno;
	uint8_t other_expected_seqno;
	uint8_t other_processed_seqno;
	uint64_t other_future_seqnos[2];
};

void init_transport_protocol(struct transport_protocol *const tp, int const sock_fd, uint8_t const flags);

/* < 0 means error, 0 means success */
int send_msg(struct transport_protocol *const tp, struct message_header const head, uint8_t const *const data);

/* < 0 means error, 0 means success, > 0 means nothing to read */
int recv_msg(struct transport_protocol *const tp, struct message_header *const head, uint8_t *const data);

/* < 0 means error, 0 means success, > 0 means POLL mask to retry */
int run_transport_protocol(struct transport_protocol *const tp, int const poll_events);

#endif

