#include "stui3/module.h"
#include "stui3.h"

#include "parent.h"
#include "child.h"

#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/socket.h>

#define MIN(A,B) (((A) > (B)) ? (A) : (B))

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
int cmd_sock_fd;

static int on_load() {
	int out_pipe[2];
	int err_pipe[2];
	int cmd_socks[2];

	if(pipe(out_pipe) < 0) {
		return -STUI3_EUPSTM;
	}
	if(pipe(err_pipe) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		return -STUI3_EUPSTM;
	}
	if(socketpair(AF_UNIX, SOCK_DGRAM, 0, cmd_socks) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		return -STUI3_EUPSTM;
	}

	if((forwarder_pid = fork()) < 0) {
		close(out_pipe[0]);
		close(out_pipe[1]);
		close(err_pipe[0]);
		close(err_pipe[1]);
		close(cmd_socks[0]);
		close(cmd_socks[1]);
		return -STUI3_EUPSTM;
	}

	if(forwarder_pid == 0) {
		/* child */
		close(out_pipe[1]);
		close(err_pipe[1]);
		close(cmd_socks[0]);
		cmd_sock_fd = cmd_socks[1];
		signal(SIGINT, SIG_IGN);
		signal(SIGTERM, SIG_IGN);
		STUI3_MODULE_NAME(forward)(out_pipe[0], err_pipe[0]);
		return -STUI3_EUPSTM;
	} else {
		/* parent */
		close(out_pipe[0]);
		close(err_pipe[0]);
		close(cmd_socks[1]);
		cmd_sock_fd = cmd_socks[0];
		stdout_fd = out_pipe[1];
		stderr_fd = err_pipe[1];
		stdout_file = fdopen(stdout_fd, stdout_mode);
		if(! stdout_file) {
			kill(forwarder_pid, SIGKILL);
			close(stdout_fd);
			close(stderr_fd);
			close(cmd_socks[0]);
			waitpid(forwarder_pid, NULL, 0);
			return -STUI3_EUPSTM;
		}
		stderr_file = fdopen(stderr_fd, stderr_mode);
		if(! stderr_file) {
			kill(forwarder_pid, SIGKILL);
			fclose(stdout_file);
			close(stderr_fd);
			close(cmd_socks[0]);
			waitpid(forwarder_pid, NULL, 0);
			return -STUI3_EUPSTM;
		}

		setvbuf(stdout_file, NULL, _IONBF, 0);
		setvbuf(stderr_file, NULL, _IONBF, 0);

		return 0;
	}
}

static void on_unload() {
	fclose(stdout_file);
	fclose(stderr_file);
	STUI3_MODULE_NAME(exec_command)("__exit", NULL, 0);
	close(cmd_sock_fd);
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
	.command = STUI3_MODULE_NAME(command),
	.unload_hook = on_unload,
};

