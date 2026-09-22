#include "comm.h"

#include "stui3.h"
#include "endian.h"
#include "crc8.h"
#include "util.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define MIN(A,B) (((A) < (B)) ? (A) : (B))

#define OWN_IN_FLIGHT(TP) ((uint8_t)((TP)->own_seqno - (TP)->own_acked_seqno))
#define OWN_FREE_SLOTS(TP) (128 - OWN_IN_FLIGHT(TP))
#define OTHER_UNPROCESSED_SLOTS(TP) ((uint8_t)((TP)->other_expected_seqno - (TP)->other_processed_seqno))

void
init_transport_protocol(struct transport_protocol *const tp, int const sock_fd, uint8_t const flags) {
	tp->state = TP_STATE_IDLE;
	tp->sock_fd = sock_fd;
	tp->flags = flags;
	tp->own_seqno = 0;
	tp->own_sent_seqno = 0;
	tp->own_acked_seqno = 0;
	tp->other_expected_seqno = 0;
	tp->other_processed_seqno = 0;
	memset(tp->other_future_seqnos, 0, sizeof(tp->other_future_seqnos));
}

int
send_msg(struct transport_protocol *const tp, struct message_header const head, uint8_t const *const data) {
	if(OWN_FREE_SLOTS(tp) == 0) {
		return -STUI3_ELIMIT;
	}
	if(head.payload_sz > MSG_PAYLOAD_MAX_LENGTH) {
		return -STUI3_EIACTN;
	}

	tp->own_headers[tp->own_seqno % 128] = head;
	tp->own_headers[tp->own_seqno % 128].magic[0] = MESSAGE_HEADER_MAGIC_0;
	tp->own_headers[tp->own_seqno % 128].magic[1] = MESSAGE_HEADER_MAGIC_1;
	tp->own_headers[tp->own_seqno % 128].magic[2] = MESSAGE_HEADER_MAGIC_2;
	tp->own_headers[tp->own_seqno % 128].magic[3] = MESSAGE_HEADER_MAGIC_3;
	tp->own_headers[tp->own_seqno % 128].seqno = tp->own_seqno;
	memcpy(tp->own_bodies[tp->own_seqno % 128], data, head.payload_sz);
	++tp->own_seqno;

	return 0;
}

int
recv_msg(struct transport_protocol *const tp, struct message_header *const head, uint8_t *const data) {
	if(OTHER_UNPROCESSED_SLOTS(tp) == 0) {
		/* TODO: scan for MSG_FLAG_INDEPENDENT messages */
		return 1;
	}

	memcpy(head, &tp->other_headers[tp->other_processed_seqno], sizeof(struct message_header));
	memcpy(data, tp->other_bodies[tp->other_processed_seqno], head->payload_sz);
	++tp->other_processed_seqno;

	return 0;
}

/* TODO: split this into some kind of run_in and run_out to avoid deadlocks */
int
run_transport_protocol(struct transport_protocol *const tp, int const poll_events) {
	ssize_t r;
	struct message_header w_head;
	uint8_t diff;
	uint8_t resync_buf[MESSAGE_HEADER_SZ_MAGIC];
	uint8_t ignore;
	uint8_t ign_buf[0x100];
	uint16_t tmp16;

	while(1) {
		switch(tp->state) {
		case TP_STATE_IDLE:
			diff = (uint8_t)(tp->own_seqno - tp->own_sent_seqno);
			if(diff > 1 && (uint8_t)(tp->own_sent_seqno - tp->own_acked_seqno) < 128 && poll_events & POLLOUT) {
#define NEXT_HEADER (tp->own_headers[(tp->own_sent_seqno+1)%128])
#define SERIALIZE_HEADER \
		NEXT_HEADER.seq_ack = tp->other_expected_seqno; \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_MAGIC, NEXT_HEADER.magic, MESSAGE_HEADER_SZ_MAGIC); \
		tmp16 = NEXT_HEADER.msg_flags; \
		if(tp->flags & TP_FLAG_SWAP_ENDIANNESS) tmp16 = byteswap16(tmp16); \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_FLAGS, &tmp16, MESSAGE_HEADER_SZ_FLAGS); \
		tmp16 = NEXT_HEADER.payload_sz; \
		if(tp->flags & TP_FLAG_SWAP_ENDIANNESS) tmp16 = byteswap16(tmp16); \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_PLD_SZ, &tmp16, MESSAGE_HEADER_SZ_PLD_SZ); \
		tmp16 = NEXT_HEADER.payload_type; \
		if(tp->flags & TP_FLAG_SWAP_ENDIANNESS) tmp16 = byteswap16(tmp16); \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_PLD_TYPE, &tmp16, MESSAGE_HEADER_SZ_PLD_TYPE); \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_SEQNO, &NEXT_HEADER.seqno, MESSAGE_HEADER_SZ_SEQNO); \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_SEQACK, &NEXT_HEADER.seq_ack, MESSAGE_HEADER_SZ_SEQACK); \
		NEXT_HEADER.crc8 = protocol_crc8(tp->w_head_buf, MESSAGE_HEADER_OFF_CRC); \
		memcpy(tp->w_head_buf+MESSAGE_HEADER_OFF_CRC, &NEXT_HEADER.crc8, MESSAGE_HEADER_SZ_CRC); \
		tp->progress = 0; \
		tp->w_item_id = (tp->own_sent_seqno + 1) % 128; \
		tp->state = TP_STATE_WRITING_HEADER;

				SERIALIZE_HEADER;
				break;
			}
			if(OTHER_UNPROCESSED_SLOTS(tp) < 128 && poll_events & POLLIN) {
				tp->progress = 0;
				tp->state = TP_STATE_READING_HEADER;
				break;
			}
			if(diff > 1 && (uint8_t)(tp->own_sent_seqno - tp->own_acked_seqno) < 128 && poll_events & POLLOUT) {
				SERIALIZE_HEADER;
				break;
#undef SERIALIZE_HEADER
#undef NEXT_HEADER
			}
			/* TODO: if TP_FLAG_SEND_ACK is set, send a bare ACK */
			return 0;
#define CHECK_R_ERRNO(R) \
		if(r == 0 || \
			(r < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) ) { \
			return -STUI3_EUPSTM; \
		} \
		if(r < 0) { \
			return (R); \
		}
		case TP_STATE_WRITING_HEADER:
			r = write(tp->sock_fd, tp->w_head_buf + tp->progress, MESSAGE_HEADER_SIZE - tp->progress);
			CHECK_R_ERRNO(POLLOUT);
			tp->progress += r;
			if(tp->progress < MESSAGE_HEADER_SIZE) {
				return POLLOUT;
			}
			tp->flags &= ~TP_FLAG_SEND_ACK;
			tp->progress = 0;
			tp->state = TP_STATE_WRITING_BODY;
			break;

		case TP_STATE_WRITING_BODY:
			r = write(tp->sock_fd,
				tp->own_bodies[tp->w_item_id] + tp->progress,
				tp->own_headers[tp->w_item_id].payload_sz - tp->progress);
			CHECK_R_ERRNO(POLLOUT);
			tp->progress += r;
			if(tp->progress < tp->own_headers[tp->w_item_id].payload_sz) {
				return POLLOUT;
			}
			tp->own_sent_seqno = tp->w_item_id;
			tp->state = TP_STATE_IDLE;
			break;

		case TP_STATE_READING_HEADER:
			r = read(tp->sock_fd, tp->w_head_buf + tp->progress, MESSAGE_HEADER_SIZE - tp->progress);
			CHECK_R_ERRNO(POLLIN);
			tp->progress += r;

			if(tp->progress < MESSAGE_HEADER_OFF_MAGIC + MESSAGE_HEADER_SZ_MAGIC) {
				return POLLIN;
			}

			memcpy(&w_head.magic, tp->w_head_buf + MESSAGE_HEADER_OFF_MAGIC, MESSAGE_HEADER_SZ_MAGIC);
			if( w_head.magic[0] != MESSAGE_HEADER_MAGIC_0 ||
				w_head.magic[1] != MESSAGE_HEADER_MAGIC_1 ||
				w_head.magic[2] != MESSAGE_HEADER_MAGIC_2 ||
				w_head.magic[3] != MESSAGE_HEADER_MAGIC_3 ) {
				tp->state = TP_STATE_RESYNC;
				break;
			}

			if(tp->progress < MESSAGE_HEADER_SIZE) {
				return POLLIN;
			}
			
#define HANDLE_ENDIAN(BITS,VAL) \
			do { \
				if(tp->flags & TP_FLAG_SWAP_ENDIANNESS) { \
					VAL = CONCAT(byteswap,BITS)(VAL); \
				} \
			} while(0);
			memcpy(&w_head.msg_flags, tp->w_head_buf + MESSAGE_HEADER_OFF_FLAGS, MESSAGE_HEADER_SZ_FLAGS);
			HANDLE_ENDIAN(16, w_head.msg_flags);
			memcpy(&w_head.payload_sz, tp->w_head_buf + MESSAGE_HEADER_OFF_PLD_SZ, MESSAGE_HEADER_SZ_PLD_SZ);
			HANDLE_ENDIAN(16, w_head.payload_sz);
			memcpy(&w_head.payload_type, tp->w_head_buf + MESSAGE_HEADER_OFF_PLD_TYPE, MESSAGE_HEADER_SZ_PLD_TYPE);
			HANDLE_ENDIAN(16, w_head.payload_type);
			memcpy(&w_head.seqno, tp->w_head_buf + MESSAGE_HEADER_OFF_SEQNO, MESSAGE_HEADER_SZ_SEQNO);
			memcpy(&w_head.seq_ack, tp->w_head_buf + MESSAGE_HEADER_OFF_SEQACK, MESSAGE_HEADER_SZ_SEQACK);
			memcpy(&w_head.crc8, tp->w_head_buf + MESSAGE_HEADER_OFF_CRC, MESSAGE_HEADER_SZ_CRC);

			if(w_head.crc8 != protocol_crc8(tp->w_head_buf, MESSAGE_HEADER_OFF_CRC)) {
				tp->state = TP_STATE_RESYNC;
				break;
			}
			if(w_head.payload_sz > MSG_PAYLOAD_MAX_LENGTH) {
				tp->state = TP_STATE_IGNORE;
				tp->progress = w_head.payload_sz;
				break;
			}

			diff = (uint8_t)(w_head.seq_ack - tp->own_acked_seqno);
			if(diff < 128) {
				tp->own_acked_seqno = w_head.seq_ack;
			} else {
				tp->state = TP_STATE_IGNORE;
				tp->progress = w_head.payload_sz;
				break;
			}

			memcpy(&tp->other_headers[w_head.seqno], &w_head, sizeof(struct message_header));
			tp->flags |= TP_FLAG_SEND_ACK;
			tp->state = TP_STATE_READING_BODY;
			tp->w_item_id = w_head.seqno;
			tp->progress = 0;
			break;

		case TP_STATE_READING_BODY:
			if(tp->progress < tp->other_headers[tp->w_item_id].payload_sz) {
				r = read(tp->sock_fd,
					tp->other_bodies[tp->w_item_id] + tp->progress,
					tp->other_headers[tp->w_item_id].payload_sz - tp->progress);
				CHECK_R_ERRNO(POLLIN);
				tp->progress += r;

				if(tp->progress < tp->other_headers[tp->w_item_id].payload_sz) {
					return POLLIN;
				}
			}

			diff = tp->other_headers[tp->w_item_id].seqno - tp->other_expected_seqno;
			
			if(diff < 64) {
				tp->other_future_seqnos[0] |= 1ULL << diff;
			}
			else {
				tp->other_future_seqnos[1] |= 1ULL << (diff - 64);
			}
			while(tp->other_future_seqnos[0] & 1) {
				++tp->other_expected_seqno;
				tp->other_future_seqnos[0] >>= 1;
				if(tp->other_future_seqnos[1] & 1) {
					tp->other_future_seqnos[0] |= 0x8000000000000000;
				}
				tp->other_future_seqnos[1] >>= 1;
			}

			tp->state = TP_STATE_IDLE;
			break;
		case TP_STATE_RESYNC:
			while(1) {
				r = recv(tp->sock_fd, resync_buf, sizeof(resync_buf), MSG_PEEK | MSG_WAITALL);
				CHECK_R_ERRNO(POLLIN);
				if(r < 4) {
					return POLLIN;
				}
				if (resync_buf[0] == MESSAGE_HEADER_MAGIC_0 &&
					resync_buf[1] == MESSAGE_HEADER_MAGIC_1 &&
					resync_buf[2] == MESSAGE_HEADER_MAGIC_2 &&
					resync_buf[3] == MESSAGE_HEADER_MAGIC_3) {
					break;
				}
				r = read(tp->sock_fd, &ignore, 1);
				CHECK_R_ERRNO(POLLIN);
			}
			tp->state = TP_STATE_IDLE;
			break;
		case TP_STATE_IGNORE:
			if(tp->progress == 0) {
				tp->state = TP_STATE_IDLE;
				break;
			}
			r = read(tp->sock_fd, ign_buf, MIN((size_t)tp->progress, (size_t)sizeof(ign_buf)));
			CHECK_R_ERRNO(POLLIN);
			tp->progress -= r;
			if(! tp->progress) {
				tp->state = TP_STATE_IDLE;
				break;
			} else {
				return POLLIN;
			}
		}
	}
}

