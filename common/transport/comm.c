#include "comm.h"

#include "stui3.h"
#include "endian.h"
#include "crc.h"
#include "util.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define MIN_REPEATS_UNTIL_RETRANSMISSION 2
#define MAX_REPEATS_UNTIL_RETRANSMISSION 4

#define OWN_TAKEN_SLOTS(TP) ((uint8_t)((TP)->own_seqno - (TP)->own_acked_seqno))
#define OWN_FREE_SLOTS(TP) (128 - OWN_TAKEN_SLOTS(TP))
#define OWN_UNSENT_SLOTS(TP) ((uint8_t)((TP)->own_seqno - (TP)->own_sent_seqno))
#define OTHER_UNPROCESSED_SLOTS(TP) ((uint8_t)((TP)->other_expected_seqno - (TP)->other_processed_seqno))

static void handle_incoming_seq_ack(struct transport_protocol *const tp, uint8_t const seq_ack);
/* < 0 means error, 0 means success, > 0 means POLL mask to retry */
static int run_read(struct transport_protocol *const tp);
static int run_write(struct transport_protocol *const tp);

void
init_transport_protocol(struct transport_protocol *const tp, int const sock_fd, uint8_t const flags, int const ticks_per_ack) {
	tp->sock_fd = sock_fd;
	tp->flags = flags;
	tp->current_ticks = 0;
	tp->ticks_per_ack = ticks_per_ack;

	tp->wstate = TP_WSTATE_IDLE;
	tp->wprogress = 0;

	tp->rstate = TP_RSTATE_IDLE;
	tp->rprogress = 0;

	tp->own_seqno = 0;
	tp->own_high_seqno = 0;
	tp->own_sent_seqno = 0;
	tp->own_acked_seqno = 0;
	
	tp->other_expected_seqno = 0;
	tp->other_processed_seqno = 0;
	memset(tp->other_future_seqnos, 0, sizeof(tp->other_future_seqnos));
	tp->ack_repeats = 0;
	tp->ack_repeat_threshold = MIN_REPEATS_UNTIL_RETRANSMISSION;
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
	if(head.payload_sz) {
		memcpy(tp->own_bodies[tp->own_seqno % 128], data, head.payload_sz);
	}
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

void
tick_protocol(struct transport_protocol *const tp) {
	if(tp->ticks_per_ack == 0) {
		tp->flags |= TP_FLAG_SEND_ACK;
	} else if(tp->ticks_per_ack > 0) {
		++tp->current_ticks;
		if(tp->current_ticks >= tp->ticks_per_ack) {
			tp->current_ticks -= tp->ticks_per_ack;
			tp->flags |= TP_FLAG_SEND_ACK;
		}
	}
}

int
run_transport_protocol(struct transport_protocol *const tp) {
	int sr, cr;

	cr = 0;
	sr = run_read(tp);
	if(sr < 0) {
		return sr;
	} else if(sr > 0) {
		cr |= sr;
	}

	sr = run_write(tp);
	if(sr < 0) {
		return sr;
	} else if(sr > 0) {
		cr |= sr;
	}

	return cr;
}

static void
handle_incoming_seq_ack(struct transport_protocol *const tp, uint8_t const seq_ack) {
	uint8_t diff;

	diff = (uint8_t)(seq_ack - tp->own_acked_seqno);
	if(diff <= (uint8_t)(tp->own_high_seqno - tp->own_acked_seqno)) {
		tp->own_acked_seqno = seq_ack;
	} else {
		return;
	}

	if(diff == 0 && tp->own_sent_seqno != tp->own_acked_seqno) {
		++tp->ack_repeats;
		if(tp->ack_repeats == tp->ack_repeat_threshold) {
			tp->own_sent_seqno = seq_ack;
			tp->ack_repeats = 0;
			if(tp->ack_repeat_threshold < MAX_REPEATS_UNTIL_RETRANSMISSION) {
				++tp->ack_repeat_threshold;
			}
		}
	} else {
		tp->ack_repeats = 0;
		tp->ack_repeat_threshold = MIN_REPEATS_UNTIL_RETRANSMISSION;
	}
}

#define CHECK_R_ERRNO(RET) \
		if(r == 0 || \
			(r < 0 && errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) ) { \
			return -STUI3_EUPSTM; \
		} \
		if(r < 0) { \
			return (RET); \
		}

static int
run_read(struct transport_protocol *const tp) {
	ssize_t r;
	struct message_header msg_head;
	struct lp_header lp_head;
	uint8_t tmp8;
	uint16_t tmp16;
	uint32_t tmp32;
	size_t additional_sz, consumed;
	int diff;
	int payload_intact;

	while(1) {
		switch(tp->rstate) {
		case TP_RSTATE_IDLE:
#define MAGIC_CMP(BUF, PREFIX) \
	((BUF)[0] == CONCAT(PREFIX,_0) && \
	(BUF)[1] == CONCAT(PREFIX,_1) && \
	(BUF)[2] == CONCAT(PREFIX,_2) && \
	(BUF)[3] == CONCAT(PREFIX,_3))

			if( tp->rprogress >= MESSAGE_HEADER_OFF_MAGIC+MESSAGE_HEADER_SZ_MAGIC &&
				MAGIC_CMP(tp->rbuf+MESSAGE_HEADER_OFF_MAGIC, MESSAGE_HEADER_MAGIC) ) {
				if(tp->rprogress < MESSAGE_HEADER_SIZE) {
					tp->rstate = TP_RSTATE_READING;
					break;
				}

				memcpy(msg_head.magic, tp->rbuf+MESSAGE_HEADER_OFF_MAGIC, MESSAGE_HEADER_SZ_MAGIC);
#define VAL16(VALUE, VNAME) \
	do { \
		if(tp->flags & TP_FLAG_SWAP_ENDIANNESS) { \
			memcpy(&tmp16, tp->rbuf+CONCAT(MESSAGE_HEADER_OFF_,VNAME), 2); \
			VALUE = byteswap16(tmp16); \
		} else { \
			memcpy(&VALUE, tp->rbuf+CONCAT(MESSAGE_HEADER_OFF_,VNAME), 2); \
		} \
	} while(0);
				VAL16(msg_head.msg_flags, FLAGS);
				VAL16(msg_head.payload_sz, PLD_SZ);
				VAL16(msg_head.payload_type, PLD_TYPE);
#undef VAL16
				memcpy(&msg_head.seqno, tp->rbuf+MESSAGE_HEADER_OFF_SEQNO, MESSAGE_HEADER_SZ_SEQNO);
				memcpy(&msg_head.seq_ack, tp->rbuf+MESSAGE_HEADER_OFF_SEQACK, MESSAGE_HEADER_SZ_SEQACK);
				memcpy(&msg_head.crc8, tp->rbuf+MESSAGE_HEADER_OFF_CRC, MESSAGE_HEADER_SZ_CRC);

				if(msg_head.crc8 != protocol_crc8(tp->rbuf, MESSAGE_HEADER_OFF_CRC)) {
					tp->rstate = TP_RSTATE_RESYNC;
					break;
				}
				if(msg_head.payload_sz > MSG_PAYLOAD_MAX_LENGTH) {
					tp->rstate = TP_RSTATE_RESYNC;
					break;
				}
				switch(msg_head.msg_flags & MSG_FLAGS_REDUNDANCY_MASK) {
				case MSG_FLAG_PAYLOAD_NO_REDUNDANCY:
					additional_sz = 0;
					break;
				case MSG_FLAG_PAYLOAD_CRC8:
					additional_sz = 1;
					break;
				case MSG_FLAG_PAYLOAD_CRC32:
					additional_sz = 4;
					break;
				default:
					/* value reserved */
					additional_sz = 0;
					break;
				}
				consumed = MESSAGE_HEADER_SIZE + msg_head.payload_sz + additional_sz;
				if(tp->rprogress < consumed) {
					tp->rstate = TP_RSTATE_READING;
					break;
				}

				payload_intact = 1;
				switch(msg_head.msg_flags & MSG_FLAGS_REDUNDANCY_MASK) {
				case MSG_FLAG_PAYLOAD_NO_REDUNDANCY:
					break;
				case MSG_FLAG_PAYLOAD_CRC8:
					memcpy(&tmp8, tp->rbuf + MESSAGE_HEADER_SIZE + msg_head.payload_sz, 1);
					if(tmp8 != protocol_crc8(tp->rbuf + MESSAGE_HEADER_SIZE, msg_head.payload_sz)) {
						payload_intact = 0;
					}
					break;
				case MSG_FLAG_PAYLOAD_CRC32:
					memcpy(&tmp32, tp->rbuf + MESSAGE_HEADER_SIZE + msg_head.payload_sz, 4);
					if(tmp32 != protocol_crc32(tp->rbuf + MESSAGE_HEADER_SIZE, msg_head.payload_sz)) {
						payload_intact = 0;
					}
					break;
				default:
					/* value reserved */
					break;
				}
				if(! payload_intact) {
					tp->rstate = TP_RSTATE_RESYNC;
					break;
				}

				handle_incoming_seq_ack(tp, msg_head.seq_ack);

				if( !(msg_head.msg_flags & MSG_FLAG_NOACK)) {
					tp->flags |= TP_FLAG_SEND_ACK;
				}

				diff = (uint8_t)(msg_head.seqno - tp->other_expected_seqno);
				if(diff < 64) {
					tp->other_future_seqnos[0] |= 1ULL << diff;
				}
				else if(diff < 128) {
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

				memcpy(&tp->other_headers[msg_head.seqno], &msg_head, sizeof(struct message_header));
				memcpy(&tp->other_bodies[msg_head.seqno], tp->rbuf+MESSAGE_HEADER_SIZE, msg_head.payload_sz);

				tp->rprogress -= consumed;
				if(tp->rprogress) {
					memmove(tp->rbuf, tp->rbuf + consumed, tp->rprogress);
				}
				break;
			} else if( tp->rprogress >= LP_HEADER_OFF_MAGIC+LP_HEADER_SZ_MAGIC &&
				MAGIC_CMP(tp->rbuf+LP_HEADER_OFF_MAGIC, LP_HEADER_MAGIC) ) {
				if(tp->rprogress < LP_HEADER_SIZE) {
					tp->rstate = TP_RSTATE_READING;
					break;
				}

				memcpy(lp_head.magic, tp->rbuf+LP_HEADER_OFF_MAGIC, LP_HEADER_SZ_MAGIC);
				memcpy(&lp_head.flags, tp->rbuf+LP_HEADER_OFF_FLAGS, LP_HEADER_SZ_FLAGS);
				memcpy(&lp_head.seq_ack, tp->rbuf+LP_HEADER_OFF_SEQACK, LP_HEADER_SZ_SEQACK);
				memcpy(&lp_head.crc8, tp->rbuf+LP_HEADER_OFF_CRC, LP_HEADER_SZ_CRC);

				if(lp_head.crc8 != protocol_crc8(tp->rbuf, LP_HEADER_OFF_CRC)) {
					tp->rstate = TP_RSTATE_RESYNC;
					break;
				}

				if(lp_head.flags & LP_FLAG_ACK_ACTIVE) {
					handle_incoming_seq_ack(tp, lp_head.seq_ack);
				}

				tp->rprogress -= LP_HEADER_SIZE;
				if(tp->rprogress) {
					memmove(tp->rbuf, tp->rbuf + LP_HEADER_SIZE, tp->rprogress);
				}
				break;
			} else if(tp->rprogress >= MESSAGE_HEADER_OFF_MAGIC+MESSAGE_HEADER_SZ_MAGIC &&
				tp->rprogress >= LP_HEADER_OFF_MAGIC+LP_HEADER_SZ_MAGIC) {
				tp->rstate = TP_RSTATE_RESYNC;
				break;
			} else {
				tp->rstate = TP_RSTATE_READING;
				break;
			}
			break;

		case TP_RSTATE_READING:
			r = recv(tp->sock_fd,
				tp->rbuf + tp->rprogress,
				sizeof(tp->rbuf) - tp->rprogress,
				MSG_DONTWAIT);
			CHECK_R_ERRNO(POLLIN);

			tp->rprogress += r;

			tp->rstate = TP_RSTATE_IDLE;
			break;

		case TP_RSTATE_RESYNC:
			if(tp->rprogress >= MESSAGE_HEADER_OFF_MAGIC+MESSAGE_HEADER_SZ_MAGIC &&
				tp->rprogress >= LP_HEADER_OFF_MAGIC+LP_HEADER_SZ_MAGIC) {
				--tp->rprogress;
				if(tp->rprogress) {
					memmove(tp->rbuf, tp->rbuf + 1, tp->rprogress);
				}
			} else {
				r = recv(tp->sock_fd, tp->rbuf, sizeof(tp->rbuf), MSG_DONTWAIT);
				CHECK_R_ERRNO(POLLIN);

				tp->rprogress += r;
			}

			if( tp->rprogress >= MESSAGE_HEADER_OFF_MAGIC+MESSAGE_HEADER_SZ_MAGIC &&
				MAGIC_CMP(tp->rbuf+MESSAGE_HEADER_OFF_MAGIC, MESSAGE_HEADER_MAGIC) ) {
				tp->rstate = TP_RSTATE_IDLE;
				break;
			} else if( tp->rprogress >= LP_HEADER_OFF_MAGIC+LP_HEADER_SZ_MAGIC &&
				MAGIC_CMP(tp->rbuf+LP_HEADER_OFF_MAGIC, LP_HEADER_MAGIC) ) {
				tp->rstate = TP_RSTATE_IDLE;
				break;
			}

			break;
		}
	}
}

static int
run_write(struct transport_protocol *const tp) {
	ssize_t r;
	uint16_t tmp16;
	struct lp_header lp_head;

	while(1) {
		switch(tp->wstate) {
		case TP_WSTATE_IDLE:
			if(OWN_UNSENT_SLOTS(tp) > 0) {
#define NEXT_HEADER (tp->own_headers[tp->own_sent_seqno%128])
#define VAL16(VALUE, VNAME) \
	do { \
		if(tp->flags & TP_FLAG_SWAP_ENDIANNESS) { \
			tmp16 = byteswap16(VALUE); \
			memcpy(tp->wbuf+CONCAT(MESSAGE_HEADER_OFF_,VNAME), &tmp16, 2); \
		} else { \
			memcpy(tp->wbuf+CONCAT(MESSAGE_HEADER_OFF_,VNAME), &VALUE, 2); \
		} \
	} while(0);
				NEXT_HEADER.seq_ack = tp->other_expected_seqno;
				memcpy(tp->wbuf+MESSAGE_HEADER_OFF_MAGIC, NEXT_HEADER.magic, MESSAGE_HEADER_SZ_MAGIC);
				VAL16(NEXT_HEADER.msg_flags, FLAGS);
				VAL16(NEXT_HEADER.payload_sz, PLD_SZ);
				VAL16(NEXT_HEADER.payload_type, PLD_TYPE);
				memcpy(tp->wbuf+MESSAGE_HEADER_OFF_SEQNO, &NEXT_HEADER.seqno, MESSAGE_HEADER_SZ_SEQNO);
				memcpy(tp->wbuf+MESSAGE_HEADER_OFF_SEQACK, &NEXT_HEADER.seq_ack, MESSAGE_HEADER_SZ_SEQACK);
				NEXT_HEADER.crc8 = protocol_crc8(tp->wbuf, MESSAGE_HEADER_OFF_CRC);
				memcpy(tp->wbuf+MESSAGE_HEADER_OFF_CRC, &NEXT_HEADER.crc8, MESSAGE_HEADER_SZ_CRC);

				memcpy(tp->wbuf+MESSAGE_HEADER_SIZE, tp->own_bodies[tp->own_sent_seqno%128], NEXT_HEADER.payload_sz);

				tp->wstate = TP_WSTATE_WRITING;
				tp->wprogress = 0;
				tp->wlength = MESSAGE_HEADER_SIZE + NEXT_HEADER.payload_sz;
				++tp->own_sent_seqno;
				if((int8_t)(tp->own_sent_seqno - tp->own_high_seqno) > 0) {
					tp->own_high_seqno = tp->own_sent_seqno;
				}
				tp->flags &= ~TP_FLAG_SEND_ACK;
				break;
#undef NEXT_HEADER
#undef VAL16
			}

			if(tp->flags & TP_FLAG_SEND_ACK) {
				lp_head.magic[0] = LP_HEADER_MAGIC_0;
				lp_head.magic[1] = LP_HEADER_MAGIC_1;
				lp_head.magic[2] = LP_HEADER_MAGIC_2;
				lp_head.magic[3] = LP_HEADER_MAGIC_3;
				lp_head.flags = LP_FLAG_ACK_ACTIVE;
				lp_head.seq_ack = tp->other_expected_seqno;

				memcpy(tp->wbuf+LP_HEADER_OFF_MAGIC, lp_head.magic, LP_HEADER_SZ_MAGIC);
				memcpy(tp->wbuf+LP_HEADER_OFF_FLAGS, &lp_head.flags, LP_HEADER_SZ_FLAGS);
				memcpy(tp->wbuf+LP_HEADER_OFF_SEQACK, &lp_head.seq_ack, LP_HEADER_SZ_SEQACK);

				lp_head.crc8 = protocol_crc8(tp->wbuf, LP_HEADER_OFF_CRC);
				memcpy(tp->wbuf+LP_HEADER_OFF_CRC, &lp_head.crc8, LP_HEADER_SZ_CRC);
				tp->wstate = TP_WSTATE_WRITING;
				tp->wprogress = 0;
				tp->wlength = LP_HEADER_SIZE;
				tp->flags &= ~TP_FLAG_SEND_ACK;
				break;
			}

			return 0;

		case TP_WSTATE_WRITING:
			r = send(tp->sock_fd,
				tp->wbuf + tp->wprogress,
				tp->wlength - tp->wprogress,
				MSG_DONTWAIT | MSG_NOSIGNAL);
			CHECK_R_ERRNO(POLLOUT);

			tp->wprogress += r;
			if(tp->wprogress < tp->wlength) {
				return POLLOUT;
			}

			tp->wstate = TP_WSTATE_IDLE;
			break;
		}
	}
}

