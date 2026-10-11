#include "transport/client-init.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>

int main() {
	int server_fd;
	struct sockaddr_un addr;
	int ret;
	struct transport_protocol tp;
	char buf[256];
	struct message_header head;

	server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	memset(&addr, 0, sizeof(struct sockaddr_un));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, "/tmp/stui3.sock", sizeof(addr.sun_path) - 1);
	addr.sun_path[sizeof(addr.sun_path) - 1] = '\0';

	connect(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un));

	if((ret = client_handshake(server_fd, &tp)) < 0) {
		fprintf(stderr, "couldn't initialize the client protocol\n");
		return 1;
	}
	if(ret > 0) {
		fprintf(stderr, "incompatible\n");
		return 1;
	}

	head.payload_type = 0;
	head.msg_flags = 0;
	while(1) {
		fgets(buf, sizeof(buf), stdin);
		if(buf[0] == '\n') break;
		head.payload_sz = strlen(buf);
		if(head.payload_sz) buf[head.payload_sz-1] = '\0';
		printf("send_msg: %d\n", send_msg(&tp, head, (uint8_t *)buf));
		tick_protocol(&tp);
		printf("run_transport_protocol: %d\n", run_transport_protocol(&tp));
	}

	return 0;
}

