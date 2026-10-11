#include "parent.h"

#include "fd-all.h"
#include "commands.h"

#include <stdio.h>
#include <string.h>

int
STUI3_MODULE_NAME(exec_command)(char const *const in, char *const out, size_t const out_sz) {
	struct STUI3_MODULE_NAME(command) cmd;
	struct STUI3_MODULE_NAME(command_response) resp;

	snprintf(cmd.string, sizeof(cmd.string), "%s", in);

	if(STUI3_MODULE_NAME(write_all)(cmd_sock_fd, &cmd, sizeof(cmd)) < 0) return 127;
	if(STUI3_MODULE_NAME(read_expected)(cmd_sock_fd, &resp, sizeof(resp)) < 0) return 127;

	snprintf(out, out_sz, "%s", resp.reason);

	return resp.rv;
}

int
STUI3_MODULE_NAME(command)(char const *const in, char *const out, size_t const out_sz) {
	if(in[0] == '_') {
		snprintf(out, out_sz, "Blocked: _ at beginning of string (%.32s)", in);
		return 127;
	}

	return STUI3_MODULE_NAME(exec_command)(in, out, out_sz);
}

