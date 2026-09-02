#include "connection.h"

#include "stui3.h"
#include "endian.h"

#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>

static int respond(struct protocol_connection *const conn, uint8_t reason);
static int send_response(struct protocol_connection *const conn);
static uint16_t byteswap16(uint16_t host);
static uint32_t byteswap32(uint32_t host);
static uint64_t byteswap64(uint64_t host);
static uint16_t identity16(uint16_t host);
static uint32_t identity32(uint32_t host);
static uint64_t identity64(uint64_t host);

void
init_connection(int const client_fd, struct protocol_connection *const conn) {
	conn->client_fd = client_fd;
	conn->hs_rw = 0;
	conn->htocs = conn->ctohs = NULL;
	conn->htocl = conn->ctohl = NULL;
	conn->htocll = conn->ctohll = NULL;
}

int
handshake_connection(struct protocol_connection *const conn) {
	ssize_t r;

	if(conn->hs_rw >= CLIENT_HANDSHAKE_SIZE) return send_response(conn);

	r = read(conn->client_fd, conn->hs_buf+conn->hs_rw, CLIENT_HANDSHAKE_SIZE-conn->hs_rw);
	if(r < 0) {
		if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return 1; /* continue */
		return -STUI3_EUPSTM;
	}
	if(r == 0) {
		return -STUI3_EUPSTM;
	}
	conn->hs_rw += r;

	if(conn->hs_rw >= CLIENT_HANDSHAKE_OFF_MAGIC + CLIENT_HANDSHAKE_SZ_MAGIC) {
		memcpy(&conn->c_handshake.magic,
			conn->hs_buf+CLIENT_HANDSHAKE_OFF_MAGIC,
			CLIENT_HANDSHAKE_SZ_MAGIC);
		if(conn->c_handshake.magic[0] != CLIENT_HANDSHAKE_MAGIC_0 ||
			conn->c_handshake.magic[1] != CLIENT_HANDSHAKE_MAGIC_1 ||
			conn->c_handshake.magic[2] != CLIENT_HANDSHAKE_MAGIC_2 ||
			conn->c_handshake.magic[3] != CLIENT_HANDSHAKE_MAGIC_3) {
			return respond(conn, SERVER_HANDSHAKE_REJECTED_MAGIC);
		}
	}

	if(conn->hs_rw >= CLIENT_HANDSHAKE_OFF_N_VERSION + CLIENT_HANDSHAKE_SZ_N_VERSION) {
		memcpy(&conn->c_handshake.network_order_version,
			conn->hs_buf+CLIENT_HANDSHAKE_OFF_N_VERSION,
			CLIENT_HANDSHAKE_SZ_N_VERSION);
		if(conn->c_handshake.network_order_version != htons(PROTOCOL_VERSION)) {
			return respond(conn, SERVER_HANDSHAKE_REJECTED_VERSION);
		}
	}

	if(conn->hs_rw == CLIENT_HANDSHAKE_SIZE) {
		memcpy(&conn->c_handshake.init_flags,
			conn->hs_buf+CLIENT_HANDSHAKE_OFF_INIT_FLAGS,
			CLIENT_HANDSHAKE_SZ_INIT_FLAGS);

		if(((conn->c_handshake.init_flags & CLIENT_HANDSHAKE_FLAG_LE) != 0) == (is_little_endian() != 0)) {
			conn->htocs = conn->ctohs = identity16;
			conn->htocl = conn->ctohl = identity32;
			conn->htocll = conn->ctohll = identity64;
		} else {
			conn->htocs = conn->ctohs = byteswap16;
			conn->htocl = conn->ctohl = byteswap32;
			conn->htocll = conn->ctohll = byteswap64;
		}

		return respond(conn, SERVER_HANDSHAKE_ACCEPTED);
	} else {
		/* need to read more */
		return 1;
	}
}

static int
respond(struct protocol_connection *const conn, uint8_t reason) {
	conn->s_handshake.magic[0] = SERVER_HANDSHAKE_MAGIC_0;
	conn->s_handshake.magic[1] = SERVER_HANDSHAKE_MAGIC_1;
	conn->s_handshake.magic[2] = SERVER_HANDSHAKE_MAGIC_2;
	conn->s_handshake.magic[3] = SERVER_HANDSHAKE_MAGIC_3;
	conn->s_handshake.network_order_version = htons(PROTOCOL_VERSION);
	conn->s_handshake.rejected = reason;
	memset(&conn->s_handshake.network_order_caps, 0, sizeof(conn->s_handshake.network_order_caps));

	memcpy(conn->hs_buf+SERVER_HANDSHAKE_OFF_MAGIC,
		&conn->s_handshake.magic,
		SERVER_HANDSHAKE_SZ_MAGIC);
	memcpy(conn->hs_buf+SERVER_HANDSHAKE_OFF_N_VERSION,
		&conn->s_handshake.network_order_version,
		SERVER_HANDSHAKE_SZ_N_VERSION);
	memcpy(conn->hs_buf+SERVER_HANDSHAKE_OFF_REJECTED,
		&conn->s_handshake.rejected,
		SERVER_HANDSHAKE_SZ_REJECTED);
	memcpy(conn->hs_buf+SERVER_HANDSHAKE_OFF_N_CAPS,
		&conn->s_handshake.network_order_caps,
		SERVER_HANDSHAKE_SZ_N_CAPS);

	conn->hs_rw = CLIENT_HANDSHAKE_SIZE;
	return send_response(conn);
}

static int
send_response(struct protocol_connection *const conn) {
	ssize_t r;

#define PROGRESS (conn->hs_rw-CLIENT_HANDSHAKE_SIZE)
	r = write(conn->client_fd, conn->hs_buf + PROGRESS, SERVER_HANDSHAKE_SIZE - PROGRESS);
#undef PROGRESS
	if(r < 0) {
		if(errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) return 1; /* continue */
		return -STUI3_EUPSTM;
	}
	if(r == 0) {
		return -STUI3_EUPSTM;
	}
	conn->hs_rw += r;

	if(conn->s_handshake.rejected == SERVER_HANDSHAKE_ACCEPTED) return 0;
	else return -STUI3_EIACTN;
}

static uint16_t
byteswap16(uint16_t host) {
	return ((host & 0x00FFU) << 8) |
		((host & 0xFF00U) >> 8);
}

static uint32_t
byteswap32(uint32_t host) {
	return ((host & 0x000000FFU) << 24) |
		((host & 0x0000FF00U) << 8) |
		((host & 0x00FF0000U) >> 8) |
		((host & 0xFF000000U) >> 24);
}

static uint64_t
byteswap64(uint64_t host) {
	return ((host & 0x00000000000000FFULL) << 56) |
		((host & 0x000000000000FF00ULL) << 40) |
		((host & 0x0000000000FF0000ULL) << 24) |
		((host & 0x00000000FF000000ULL) << 8) |
		((host & 0x000000FF00000000ULL) >> 8) |
		((host & 0x0000FF0000000000ULL) >> 24) |
		((host & 0x00FF000000000000ULL) >> 40) |
		((host & 0xFF00000000000000ULL) >> 56);
}

static uint16_t
identity16(uint16_t host) {
	return host;
}

static uint32_t
identity32(uint32_t host) {
	return host;
}

static uint64_t
identity64(uint64_t host) {
	return host;
}

