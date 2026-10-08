#include "handle-client.h"

#include "stui3.h"
#include "core/logf.h"

void init_client_context(int const client_fd, struct client_context *const ctx) {
	ctx->state = CL_CTX_STATE_HANDSHAKE;
	init_connection(client_fd, &ctx->conn);
}

int handle_client(struct client_context *const ctx) {
	int ret;
	struct message_header head;
	uint8_t data[MSG_PAYLOAD_MAX_LENGTH];

	while(1) {
		switch(ctx->state) {
		case CL_CTX_STATE_HANDSHAKE:
			ret = handshake_connection(&ctx->conn);
			if(ret != 0) return ret;
			/* TODO: change the ticks per ack */
			init_transport_protocol(&ctx->tp, ctx->conn.client_fd, 0, 4);
			ctx->state = CL_CTX_STATE_IDLE;
			break;
		case CL_CTX_STATE_IO:
			ret = run_transport_protocol(&ctx->tp);
			if(ret != 0) return ret;
			ctx->state = CL_CTX_STATE_IDLE;
			break;
		case CL_CTX_STATE_IDLE:
			while(1) {
				tick_protocol(&ctx->tp);
				ret = recv_msg(&ctx->tp, &head, data);
				if(ret > 0) break;
				logf_stdout("Received a message of type %u and length %u\n", head.payload_type, head.payload_sz);
				logf_stdout("Message interpreted as a string: %s\n", data);
			}

			ctx->state = CL_CTX_STATE_IO;
			break;
		}
	}

	return -STUI3_EIACTN;
}

