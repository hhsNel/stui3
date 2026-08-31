#include "core/setup-daemon.h"
#include "symbols/symbol-table.h"
#include "core/logf.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>

int main(int argc, char **argv) {
	unsigned int i;
	int ret;
	int socket_fd;
	struct sockaddr_un socket_address;

	char *sock_path = NULL;

	if((ret = init_symbols()) < 0) exit(ret);

	for(i = 1; i < (unsigned int)argc; ++i) {
#define ARG(STR) (strcmp(argv[i], STR) == 0)
#define ARG_CHECK do { if(i + 1 >= (unsigned int)argc) {fprintf(stderr, "Expected another argument after %s\n", argv[i]); exit(1);} } while(0);
		if(argv[i][0] != '-') {
			fprintf(stderr, "Expected a flag, got: %s\n", argv[i]);
			exit(1);
		} else if(ARG("-v") || ARG("--version")) {
			printf("stui3 server\nABI version: %d\n", (int)ABI_VERSION);
			exit(0);
		} else if(ARG("-h") || ARG("--help")) {
			printf("%s [options...]\n", argv[0]);
			printf("Options:\n");
			printf("\t%-48s %s\n", "-v | --version", "Display version information");
			printf("\t%-48s %s\n", "-h | --help", "Display this information");
			printf("\t%-48s %s\n", "-s <PATH> | -sock <PATH> | --sock <PATH>", "Set the socket path to <PATH>");
			exit(0);
		} else if(ARG("-s") || ARG("-sock") || ARG("--sock")) {
			ARG_CHECK;
			sock_path = argv[++i];
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			exit(1);
		}
#undef ARG
	}

	if(! sock_path) sock_path = "/tmp/stui3.sock";

	logf_stdout("stui3 server starting up...\n");
	logf_stdout("The socket path is: %s\n", sock_path);

	if(access(sock_path, F_OK) == 0) {
		logf_stderr("Socket path already exists: %s", sock_path);
		exit(1);
	}

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		logf_stderr("Could not open a socket: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	logf_stdout("Opened a socket (fd: %d)\n", socket_fd);

	memset(&socket_address, 0, sizeof(struct sockaddr_un));
	socket_address.sun_family = AF_UNIX;
	strncpy(socket_address.sun_path, sock_path, sizeof(socket_address.sun_path) - 1);
	if(bind(socket_fd, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0) {
		logf_stderr("Could not bind: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	logf_stdout("Bound socket to path %s\n", sock_path);

	logf_stderr("Operation (err)...\n");
	logf_stdout("Operation (out)...\n");

	logf_stdout("Shutting down the socket\n");
	unlink(sock_path);
	close(socket_fd);

	logf_stdout("Shutting down symbol resolution\n");
	shutdown_symbols();
}

