#ifndef COMMON_TRANSPORT_COMM_H
#define COMMON_TRANSPORT_COMM_H

#include "protocol.h"
#include "util.h"

#include <stddef.h>

#define TP_FLAG_SEND_ACK 0x80
#define TP_FLAG_NO_READ 0x40
#define TP_FLAG_SWAP_ENDIANNESS 0x01

enum transport_protocol_wstate {
	TP_WSTATE_IDLE,
	TP_WSTATE_WRITING,
};

enum transport_protocol_rstate {
	TP_RSTATE_IDLE,
	TP_RSTATE_READING,
	TP_RSTATE_RESYNC,
};

struct transport_protocol {
	int sock_fd;
	uint8_t flags;
	int current_ticks, ticks_per_ack;

	enum transport_protocol_wstate wstate;
	size_t wprogress, wlength;
	uint8_t wbuf[PROTOCOL_MAX_TRANSMISSION_SIZE];

	enum transport_protocol_rstate rstate;
	size_t rprogress;
	uint8_t rbuf[PROTOCOL_MAX_TRANSMISSION_SIZE + 4]; /* 4 additional bytes for resync */

	struct message_header own_headers[128];
	uint8_t own_bodies[128][MSG_PAYLOAD_MAX_LENGTH];

	struct message_header other_headers[256];
	uint8_t other_bodies[256][MSG_PAYLOAD_MAX_LENGTH];

	uint8_t own_seqno;
	uint8_t own_high_seqno;
	uint8_t own_sent_seqno;
	uint8_t own_acked_seqno;

	uint8_t other_expected_seqno;
	uint8_t other_processed_seqno;
	uint64_t other_future_seqnos[2];
	uint8_t ack_repeats;
	uint8_t ack_repeat_threshold;
};

void init_transport_protocol(struct transport_protocol *const tp, int const sock_fd, uint8_t const flags, int const ticks_per_ack);

/* < 0 means error, 0 means success */
int send_msg(struct transport_protocol *const tp, struct message_header const head, uint8_t const *const data);

/* < 0 means error, 0 means success, > 0 means nothing to read */
int recv_msg(struct transport_protocol *const tp, struct message_header *const head, uint8_t *const data);

void tick_protocol(struct transport_protocol *const tp);

/* < 0 means error, 0 means success, > 0 means POLL mask to retry */
int run_transport_protocol(struct transport_protocol *const tp);

#endif

