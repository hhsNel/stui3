#include "handle-client.h"

#include "stui3.h"

void init_client_context(int const client_fd, struct client_context *const ctx) {
	ctx->state = CL_CTX_STATE_HANDSHAKE;
	init_connection(client_fd, &ctx->conn);
}

int handle_client(struct client_context *const ctx) {
	switch(ctx->state) {
	case CL_CTX_STATE_HANDSHAKE:
		return handshake_connection(&ctx->conn);
	}

	return -STUI3_EIACTN;
}

