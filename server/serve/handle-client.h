#ifndef SERVER_SERVE_HANDLE_CLIENT_H
#define SERVER_SERVE_HANDLE_CLIENT_H

#include "transport/connection.h"
#include "transport/comm.h"

enum client_context_state {
	CL_CTX_STATE_HANDSHAKE,
	CL_CTX_STATE_IO,
	CL_CTX_STATE_IDLE,
};

struct client_context {
	enum client_context_state state;
	struct protocol_connection conn;
	struct transport_protocol tp;
};

void init_client_context(int const client_fd, struct client_context *const ctx);
/* < 0 means error, 0 means success, > 0 means continue */
int handle_client(struct client_context *const ctx);

#endif

