#include "stui3/module.h"
#include "stui3.h"

#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define MAX(A,B) (((A) > (B)) ? (A) : (B))

static FILE *stdout_file;
static int stdout_fd;
static char stdout_mode[] = "w";
static FILE *stderr_file;
static int stderr_fd;
static char stderr_mode[] = "w";

static stui3_module_symbol symbols[] = {
	"stdout_file",
	"stdout_fd",
	"stdout_mode",
	"stderr_file",
	"stderr_fd",
	"stderr_mode",
};
static void *pointers[] = {
	&stdout_file,
	&stdout_fd,
	&stdout_mode,
	&stderr_file,
	&stderr_fd,
	&stderr_mode,
};

static pid_t forwarder_pid;
static int mosi_pipe[2];
static int miso_pipe[2];

static void write_all(int fd, void const *buf, size_t sz) {
	int ret;
	unsigned int retries;

	retries = 8;
	while(sz > 0) {
		if((ret = write(fd, buf, sz)) < 0) {
			if(retries--) continue;
			else return;
		}
		sz -= ret;
		buf = (char *)buf + ret;
	}
}

static void read_expected(int fd, void *buf, size_t sz) {
	int ret;
	unsigned int retries;

	retries = 8;
	while(sz > 0) {
		if((ret = read(fd, buf, sz)) < 0) {
			if(retries--) continue;
			else return;
		}
		sz -= ret;
		buf = (char *)buf + ret;
	}
}

enum write_target_type {
	WT_TYPE_FD,
	WT_TYPE_FILE,
};
struct write_target {
	enum write_target_type type;
	union {
		int fd;
		char path[256];
	} data;
};
struct write_designator {
	struct write_target tgts[16];
	uint8_t num_targets;
};
static void handle_write(struct write_designator *wd, char *buf, size_t sz) {
	unsigned int i;

	if(wd->num_targets > sizeof(wd->tgts)/sizeof(*wd->tgts)) wd->num_targets = sizeof(wd->tgts)/sizeof(*wd->tgts);

	for(i = 0; i < wd->num_targets; ++i) {
		switch(wd->tgts[i].type) {
		case WT_TYPE_FD:
			write_all(wd->tgts[i].data.fd, buf, sz);
			break;
		case WT_TYPE_FILE:
			/* TODO */
			exit(1);
			break;
		}
	}
}

static void forward(int stdout_fd, int stderr_fd) {
	char buf[0x1000];
	struct pollfd fds[3];
	int ret;
	uint16_t len;
	struct write_designator wd_out = { { {.type=WT_TYPE_FD,.data.fd=STDOUT_FILENO} }, 1 };
	struct write_designator wd_err = { { {.type=WT_TYPE_FD,.data.fd=STDERR_FILENO} }, 1 };

	fds[0].fd = stdout_fd;
	fds[1].fd = stderr_fd;
	fds[2].fd = mosi_pipe[0];
	fds[0].events = fds[1].events = fds[2].events = POLLIN;
	while(1) {
		if(poll(fds, sizeof(fds)/sizeof(*fds), -1) > 0) {
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
				read_expected(mosi_pipe[0], &len, sizeof(len));
				if(len > sizeof(buf)) {
					while(len > sizeof(buf)) {
						read_expected(mosi_pipe[0], buf, sizeof(buf));
						len -= sizeof(buf);
					}
					read_expected(mosi_pipe[0], buf, len);
					ret = 1;
					snprintf(buf, sizeof(buf), "Too long: %u", (unsigned int)len);
					len = strlen(buf);
					write_all(miso_pipe[1], &ret, sizeof(ret));
					write_all(miso_pipe[1], &len, sizeof(len));
					write_all(miso_pipe[1], buf, len);
				} else {
					/* handle command */
					read_expected(mosi_pipe[0], buf, len);
					if(strcmp(buf, "__exit") == 0) {
						while((ret = read(stdout_fd, buf, sizeof(buf))) > 0) handle_write(&wd_out, buf, ret);
						while((ret = read(stderr_fd, buf, sizeof(buf))) > 0) handle_write(&wd_err, buf, ret);
						ret = 0;
						len = 0;
						write_all(miso_pipe[1], &ret, sizeof(ret));
						write_all(miso_pipe[1], &len, sizeof(len));
						exit(0);
					} else {
						ret = 1;
						snprintf(buf, sizeof(buf), "Unknown command");
						len = strlen(buf);
						write_all(miso_pipe[1], &ret, sizeof(ret));
						write_all(miso_pipe[1], &len, sizeof(len));
						write_all(miso_pipe[1], buf, len);
					}
				}
			}
		}
	}
}

static int on_load() {
	int out_pipe[2];
	int err_pipe[2];

	if(pipe(out_pipe) < 0) {
		return -STUI3_EUPSTM;
	}
	if(pipe(err_pipe) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		return -STUI3_EUPSTM;
	}
	if(pipe(mosi_pipe) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		return -STUI3_EUPSTM;
	}
	if(pipe(miso_pipe) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		close(mosi_pipe[0]);
		close(mosi_pipe[1]);
		return -STUI3_EUPSTM;
	}

	if((forwarder_pid = fork()) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		close(mosi_pipe[0]);
		close(mosi_pipe[1]);
		close(miso_pipe[0]);
		close(miso_pipe[1]);
		return -STUI3_EUPSTM;
	}

	if(forwarder_pid == 0) {
		/* child */
		close(out_pipe[1]);
		close(err_pipe[1]);
		close(mosi_pipe[1]);
		close(miso_pipe[0]);
		forward(out_pipe[0], err_pipe[0]);
		return -STUI3_EUPSTM;
	} else {
		/* parent */
		close(out_pipe[0]);
		close(err_pipe[0]);
		close(mosi_pipe[0]);
		close(miso_pipe[1]);
		stdout_fd = out_pipe[1];
		stderr_fd = err_pipe[1];
		stdout_file = fdopen(stdout_fd, stdout_mode);
		if(! stdout_file) {
			kill(forwarder_pid, SIGTERM);
			close(stdout_fd);
			close(stderr_fd);
			close(mosi_pipe[1]);
			close(miso_pipe[0]);
			waitpid(forwarder_pid, NULL, 0);
			return -STUI3_EUPSTM;
		}
		stderr_file = fdopen(stderr_fd, stderr_mode);
		if(! stderr_file) {
			kill(forwarder_pid, SIGTERM);
			fclose(stdout_file);
			close(stderr_fd);
			close(mosi_pipe[1]);
			close(miso_pipe[0]);
			waitpid(forwarder_pid, NULL, 0);
			return -STUI3_EUPSTM;
		}

		setvbuf(stdout_file, NULL, _IONBF, 0);
		setvbuf(stderr_file, NULL, _IONBF, 0);

		return 0;
	}
}

static int exec_command(char const *const in, char *const out, size_t const out_sz) {
	char buf[0x1000];
	uint16_t len;
	int ret;

	len = strlen(in) + 1;
	write_all(mosi_pipe[1], &len, sizeof(len));
	write_all(mosi_pipe[1], in, len);
	read_expected(miso_pipe[0], &ret, sizeof(ret));
	read_expected(miso_pipe[0], &len, sizeof(len));
	if(len > sizeof(buf)) {
		read_expected(miso_pipe[0], buf, sizeof(buf));
		len -= sizeof(buf);
		if(out) {
			strncpy(out, buf, MAX(out_sz-1, sizeof(buf)));
			out[out_sz-1] = '\0';
		}
		while(len > sizeof(buf)) {
			read_expected(miso_pipe[0], buf, sizeof(buf));
			len -= sizeof(buf);
		}
		read_expected(miso_pipe[0], buf, len);
	} else {
		read_expected(miso_pipe[0], buf, len);
		if(out) {
			strncpy(out, buf, MAX(out_sz-1, sizeof(buf)));
			out[out_sz-1] = '\0';
		}
	}

	return ret;
}

static int command(char const *const in, char *const out, size_t const out_sz) {
	if(in[0] == '_') {
		snprintf(out, out_sz, "Blocked: _ at beginning of string (%.32s)", in);
		return 1;
	}

	return exec_command(in, out, out_sz);
}

static void on_unload() {
	fclose(stdout_file);
	fclose(stderr_file);
	exec_command("__exit", NULL, 0);
	close(mosi_pipe[1]);
	close(miso_pipe[0]);
	waitpid(forwarder_pid, NULL, 0);
}

struct stui3_module_description STUI3_MODULE_NAME(stui3_module) = {
	.magic = { STUI3_MODULE_DESCRIPTION_MAGIC_0, STUI3_MODULE_DESCRIPTION_MAGIC_1, STUI3_MODULE_DESCRIPTION_MAGIC_2, STUI3_MODULE_DESCRIPTION_MAGIC_3 },
	.minimum_abi = 5,
	.deprecation_abi = ABI_VERSION + 1,
	.name = "dynlog",
	.description = "Provides symbols necessary for logging",
	.symbols = symbols,
	.pointers = pointers,
	.num_exports = sizeof(symbols)/sizeof(*symbols),
	.load_hook = on_load,
	.command = command,
	.unload_hook = on_unload,
};

