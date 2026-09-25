#include "transport/comm.h"
#include "stui3.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <poll.h>
#include <unistd.h>
#include <inttypes.h>

static struct transport_protocol tps[2];
static int fds[2];

static char *state_names[] = {
	[TP_STATE_IDLE] = "IDLE",
	[TP_STATE_WRITING_HEADER] = "WRITING_HEADER",
	[TP_STATE_WRITING_BODY] = "WRITING_BODY",
	[TP_STATE_READING_HEADER] = "READING_HEADER",
	[TP_STATE_READING_BODY] = "READING_BODY",
	[TP_STATE_WRITING_LP] = "WRITING_LP",
	[TP_STATE_READING_LP] = "READING_LP",
	[TP_STATE_RESYNC] = "RESYNC",
	[TP_STATE_IGNORE] = "IGNORE",
};

static char *
stui3errstr(int e) {
	if(e < 0) e = -e;
	switch(e) {
	case 0:
		return "success";
	case STUI3_ENOENT:
		return "ENOENT";
	case STUI3_EUPSTM:
		return "EUPSTM";
	case STUI3_EIACTN:
		return "EIACTN";
	case STUI3_EIDATA:
		return "EIDATA";
	case STUI3_ELIMIT:
		return "ELIMIT";
	case STUI3_ECHECK:
		return "ECHECK";
	default:
		return "unknown error";
	}
}

static void
print_ret(int r) {
	if(r < 0) {
		printf("%d (%s)\n", r, stui3errstr(r));
	} else if(r == 0) {
		printf("0\n");
	} else {
		printf("%d (", r);
		if(r & POLLIN) printf("POLLIN ");
		if(r & POLLOUT) printf("POLLOUT ");
		printf(")\n");
	}
}

static void
info(struct transport_protocol const *const tp) {
	printf("\tstate: %s\n", state_names[tp->state]);
	printf("\town_seqno: %" PRIu8 "\n", tp->own_seqno);
	printf("\town_high_seqno: %" PRIu8 "\n", tp->own_high_seqno);
	printf("\town_sent_seqno: %" PRIu8 "\n", tp->own_sent_seqno);
	printf("\town_acked_seqno: %" PRIu8 "\n", tp->own_acked_seqno);
	printf("\tother_expected_seqno: %" PRIu8 "\n", tp->other_expected_seqno);
	printf("\tack_repeats: %" PRIu8 "\n", tp->ack_repeats);
	printf("\tack_repeat_threshold: %" PRIu8 "\n", tp->ack_repeat_threshold);
}

int main() {
	char buf[2048];
	char arg[64];
	char c;
	struct message_header head;
	struct pollfd pfd;
	int r;

	head.msg_flags = 0;
	head.payload_type = 0;
	pfd.events = POLLIN | POLLOUT;

	if(socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds) < 0) {
		perror("socketpair");
		exit(1);
	}
	init_transport_protocol(&tps[0], fds[0], 0);
	init_transport_protocol(&tps[1], fds[1], 0);

	puts("ready");

	while(1) {
		if(! (r=scanf("%2047[^\n]", buf)) || r < 0) exit(1);
		if(! scanf("%c", &c)) exit(1);

#define ERROR() \
	do { puts("error"); continue; } while(0);

		if(strncmp(buf, "help", 4) == 0) {
			puts("help|exit|send0 <str>|send1 <str>|recv0|recv1|run0|run1|junk0|junk1|info0|info1");
		} else if(strncmp(buf, "send0", 5) == 0) {
			sscanf(buf, "send0 %63s", arg);
			head.payload_sz = strlen(arg);
			printf("ok "); print_ret(send_msg(&tps[0], head, (uint8_t *)arg));
		} else if(strncmp(buf, "send1", 5) == 0) {
			sscanf(buf, "send1 %63s", arg);
			head.payload_sz = strlen(arg);
			printf("ok "); print_ret(send_msg(&tps[1], head, (uint8_t *)arg));
		} else if(strncmp(buf, "recv0", 5) == 0) {
			printf("ok "); print_ret(r = recv_msg(&tps[0], &head, (uint8_t *)arg));
			if(r == 0) {
				arg[head.payload_sz] = 0;
				printf("\tpayload: %s\n", arg);
			}
		} else if(strncmp(buf, "recv1", 5) == 0) {
			printf("ok "); print_ret(r = recv_msg(&tps[1], &head, (uint8_t *)arg));
			if(r == 0) {
				arg[head.payload_sz] = 0;
				printf("\tpayload: %s\n", arg);
			}
		} else if(strncmp(buf, "run0", 4) == 0) {
			pfd.fd = fds[0];
			poll(&pfd, 1, 0);
			printf("ok "); print_ret(run_transport_protocol(&tps[0], pfd.revents));
		} else if(strncmp(buf, "run1", 4) == 0) {
			pfd.fd = fds[1];
			poll(&pfd, 1, 0);
			printf("ok "); print_ret(run_transport_protocol(&tps[1], pfd.revents));
		} else if(strncmp(buf, "junk0", 5) == 0) {
			c = -1;
			printf("ok %d\n", (int)write(fds[0], &c, 1));
		} else if(strncmp(buf, "junk1", 5) == 0) {
			c = -1;
			printf("ok %d\n", (int)write(fds[1], &c, 1));
		} else if(strncmp(buf, "info0", 5) == 0) {
			printf("ok\n");
			info(&tps[0]);
		} else if(strncmp(buf, "info1", 5) == 0) {
			printf("ok\n");
			info(&tps[1]);
		} else if(strncmp(buf, "exit", 4) == 0) {
			exit(0);
		} else {
			printf("unknown command: %s\n", buf);
		}
	}
}

