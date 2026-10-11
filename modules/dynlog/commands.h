#ifndef DYNLOG_COMMANDS_H
#define DYNLOG_COMMANDS_H

extern int cmd_sock_fd;

struct STUI3_MODULE_NAME(command) {
	char string[512];
};

struct STUI3_MODULE_NAME(command_response) {
	int rv;
	char reason[512];
};

#endif

