#include "child.h"

#include "core/setup-daemon.h"

#include "commands.h"
#include "fd-all.h"

#include <stdio.h>
#include <poll.h>
#include <syslog.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

enum write_target_type {
	WT_TYPE_FD,
	WT_TYPE_SYSLOG,
	WT_TYPE_CTTY,
};

struct write_target {
	enum write_target_type type;
	union {
		int fd;
		int log_type;
	} data;
};

struct write_designator {
	char prefix[16];
	struct write_target tgts[16];
	uint8_t num_targets;
};

static void handle_write(struct write_designator *wd, char *buf, size_t sz);

void
STUI3_MODULE_NAME(forward)(int stdout_fd, int stderr_fd) {
	char buf[0x1000];
	struct pollfd fds[3];
	int ret;
	struct STUI3_MODULE_NAME(command) cmd;
	struct STUI3_MODULE_NAME(command_response) resp;
	struct write_designator wd_out = { "[OUT] ", { {.type=WT_TYPE_CTTY} }, 1 };
	struct write_designator wd_err = { "[ERR] ", { {.type=WT_TYPE_CTTY} }, 1 };
	struct write_designator *modified_wd;
	unsigned int closed = 0;

	fds[0].fd = stdout_fd;
	fds[1].fd = stderr_fd;
	fds[2].fd = cmd_sock_fd;
	fds[0].events = fds[1].events = fds[2].events = POLLIN;

#define DRAIN_ALL \
	while((ret = read(stdout_fd, buf, sizeof(buf))) > 0) handle_write(&wd_out, buf, ret); \
	while((ret = read(stderr_fd, buf, sizeof(buf))) > 0) handle_write(&wd_err, buf, ret);

	while(1) {
		ret = poll(fds, sizeof(fds)/sizeof(*fds), -1);
		if(ret < 0) {
			if(errno == EINTR) continue;
			break;
		}
		if(ret == 0) continue;

		if(fds[0].revents & POLLIN) {
			ret = read(fds[0].fd, buf, sizeof(buf));
			if(ret > 0) {
				handle_write(&wd_out, buf, ret);
			}
		}
		if(fds[1].revents & POLLIN) {
			ret = read(fds[1].fd, buf, sizeof(buf));
			if(ret > 0) {
				handle_write(&wd_err, buf, ret);
			}
		}
		if(fds[2].revents & POLLIN) {
			if(STUI3_MODULE_NAME(read_expected)(cmd_sock_fd, &cmd, sizeof(cmd)) < 0) return;
			/* handle command */
			if(strcmp(cmd.string, "__exit") == 0) {
				DRAIN_ALL;
				resp.rv = 0;
				resp.reason[0] = '\0';
				if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
				exit(0);
			} else if(strncmp(cmd.string, "tgt_out ", 8) == 0 || strncmp(cmd.string, "tgt_err ", 8) == 0) {
				if(strncmp(cmd.string, "tgt_out ", 8) == 0) modified_wd = &wd_out;
				else modified_wd = &wd_err;

				if(modified_wd->num_targets >= sizeof(modified_wd->tgts)/sizeof(*modified_wd->tgts)) {
					resp.rv = 1;
					snprintf(resp.reason, sizeof(resp.reason), "Out of target slots");
					if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
				}

				if(strncmp(cmd.string+8, "fd ", 2) == 0) {
					modified_wd->tgts[modified_wd->num_targets++] = (struct write_target){WT_TYPE_FD,{.fd=atoi(cmd.string+11)}};
					resp.rv = 0;
					snprintf(resp.reason, sizeof(resp.reason), "Target set to fd %d", atoi(cmd.string+10));
					if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
				} else if(strncmp(cmd.string+8, "syslog ", 7) == 0) {
					modified_wd->tgts[modified_wd->num_targets++] = (struct write_target){WT_TYPE_SYSLOG, {.log_type=(cmd.string[15]=='o'?LOG_INFO:LOG_ERR)}};
					resp.rv = 0;
					snprintf(resp.reason, sizeof(resp.reason), "Target set to syslog level %s", cmd.string[15]=='o'?"LOG_INFO":"LOG_ERR");
					if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
				} else if(strncmp(cmd.string+8, "ctty", 4) == 0) {
					modified_wd->tgts[modified_wd->num_targets++] = (struct write_target){.type=WT_TYPE_CTTY};
					resp.rv = 0;
					snprintf(resp.reason, sizeof(resp.reason), "Target set to the controlling terminal");
					if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
				} else {
					resp.rv = 1;
					snprintf(resp.reason, sizeof(resp.reason), "Unknown target");
					if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
				}
			} else {
				resp.rv = 1;
				snprintf(resp.reason, sizeof(resp.reason), "Unknown command");
				if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return;
			}
		}

#define CHECK_ERR(I) \
		if(fds[I].revents & (POLLERR | POLLHUP)) { \
			close(fds[I].fd); \
			fds[I].fd = -1; \
			++closed; \
			if(closed == sizeof(fds)/sizeof(*fds)) break; \
		}
		CHECK_ERR(0);
		CHECK_ERR(1);
		CHECK_ERR(2);
	}

	DRAIN_ALL;
#undef DRAIN_ALL
	_exit(0);
}

static void
handle_write(struct write_designator *wd, char *buf, size_t sz) {
	unsigned int i;

	if(wd->num_targets > sizeof(wd->tgts)/sizeof(*wd->tgts)) wd->num_targets = sizeof(wd->tgts)/sizeof(*wd->tgts);

	for(i = 0; i < wd->num_targets; ++i) {
		switch(wd->tgts[i].type) {
		case WT_TYPE_FD:
			STUI3_MODULE_NAME(write_all)(wd->tgts[i].data.fd, wd->prefix, strlen(wd->prefix));
			STUI3_MODULE_NAME(write_all)(wd->tgts[i].data.fd, buf, sz);
			break;
		case WT_TYPE_SYSLOG:
			syslog(wd->tgts[i].data.log_type, "%s%.*s", wd->prefix, (int)sz, buf);
			break;
		case WT_TYPE_CTTY:
			STUI3_MODULE_NAME(write_all)(ctty_fd, wd->prefix, strlen(wd->prefix));
			STUI3_MODULE_NAME(write_all)(ctty_fd, buf, sz);
			break;
		}
	}
}

