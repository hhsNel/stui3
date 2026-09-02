#include "protocol/comm.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>

int main() {
	int server_fd;
	struct sockaddr_un addr;
	int ret;
	struct server_handshake s_hs;

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/tmp/stui3.sock", sizeof(addr.sun_path) - 1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	connect(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));

	if(send_client_handshake(server_fd) < 0) {
		fprintf(stderr, "couldn't send the client handshake\n");
		return 1;
	}
	if((ret = recv_server_handshake(server_fd, &s_hs)) < 0) {
		fprintf(stderr, "couldn't recv the server handshake\n");
		return 1;
	}
	if(ret > 0) {
		fprintf(stderr, "incompatible\n");
		return 1;
	}

	return 0;
}

