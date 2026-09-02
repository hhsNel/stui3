#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_VERSION (0)

#define CRC8_POLYNOMIAL (0x07)
#define CRC8_INITIAL (0x00)
/* no reflection */
#define CRC8_XOR (0x00)

#define CLIENT_HANDSHAKE_MAGIC_0 (0xB3)
#define CLIENT_HANDSHAKE_MAGIC_1 (0x78)
#define CLIENT_HANDSHAKE_MAGIC_2 (0x6C)
#define CLIENT_HANDSHAKE_MAGIC_3 (0x72)

#define CLIENT_HANDSHAKE_FLAG_BE (0x00) /* byte order for everything except the handshake */
#define CLIENT_HANDSHAKE_FLAG_LE (0x01) /* byte order for everything except the handshake */

#define CLIENT_HANDSHAKE_OFF_MAGIC (0)
#define CLIENT_HANDSHAKE_SZ_MAGIC (4)
#define CLIENT_HANDSHAKE_OFF_N_VERSION (4)
#define CLIENT_HANDSHAKE_SZ_N_VERSION (2)
#define CLIENT_HANDSHAKE_OFF_INIT_FLAGS (6)
#define CLIENT_HANDSHAKE_SZ_INIT_FLAGS (1)
#define CLIENT_HANDSHAKE_SIZE \
	(CLIENT_HANDSHAKE_SZ_MAGIC + \
	CLIENT_HANDSHAKE_SZ_N_VERSION + \
	CLIENT_HANDSHAKE_SZ_INIT_FLAGS)
struct client_handshake {
	uint8_t magic[4];
	uint16_t network_order_version; /* protocol version in network byte order */
	uint8_t init_flags; /* flags describing the client */
};

#define SERVER_HANDSHAKE_MAGIC_0 (0xB2)
#define SERVER_HANDSHAKE_MAGIC_1 (0xFB)
#define SERVER_HANDSHAKE_MAGIC_2 (0x37)
#define SERVER_HANDSHAKE_MAGIC_3 (0x86)

#define SERVER_HANDSHAKE_ACCEPTED (0x00)
#define SERVER_HANDSHAKE_REJECTED_MAGIC (0x01)
#define SERVER_HANDSHAKE_REJECTED_VERSION (0x02)
#define SERVER_HANDSHAKE_REJECTED_FLAGS (0x03)

#define SERVER_HANDSHAKE_OFF_MAGIC (0)
#define SERVER_HANDSHAKE_SZ_MAGIC (4)
#define SERVER_HANDSHAKE_OFF_N_VERSION (4)
#define SERVER_HANDSHAKE_SZ_N_VERSION (2)
#define SERVER_HANDSHAKE_OFF_REJECTED (6)
#define SERVER_HANDSHAKE_SZ_REJECTED (1)
#define SERVER_HANDSHAKE_OFF_N_CAPS (7)
#define SERVER_HANDSHAKE_SZ_N_CAPS (16)
#define SERVER_HANDSHAKE_SIZE \
	(SERVER_HANDSHAKE_SZ_MAGIC + \
	SERVER_HANDSHAKE_SZ_N_VERSION + \
	SERVER_HANDSHAKE_SZ_REJECTED + \
	SERVER_HANDSHAKE_SZ_N_CAPS)
struct server_handshake {
	uint8_t magic[4];
	uint16_t network_order_version; /* protocol version in network byte order */
	uint8_t rejected; /* zero = proceed, nonzero = reason */
	uint32_t network_order_caps[4]; /* the server supports / doesn't support */
};

#define MESSAGE_HEADER_MAGIC_0 (0x7A)
#define MESSAGE_HEADER_MAGIC_1 (0xF7)
#define MESSAGE_HEADER_MAGIC_2 (0xA7)
#define MESSAGE_HEADER_MAGIC_3 (0xB7)

#define MSG_FLAG_INDEPENDENT (0x0001) /* the server is allowed to process this message even if it's not the next unACKed one */
#define MSG_FLAG_PAYLOAD_CRC8 (0x0002) /* after the payload (not counting toward the payload_sz) there's a CRC8 checksum of the payload */

#define MESSAGE_HEADER_OFF_MAGIC (0)
#define MESSAGE_HEADER_SZ_MAGIC (4)
#define MESSAGE_HEADER_OFF_FLAGS (4)
#define MESSAGE_HEADER_SZ_FLAGS (2)
#define MESSAGE_HEADER_OFF_PLD_SZ (6)
#define MESSAGE_HEADER_SZ_PLD_SZ (2)
#define MESSAGE_HEADER_OFF_PLD_TYPE (8)
#define MESSAGE_HEADER_SZ_PLD_TYPE (2)
#define MESSAGE_HEADER_OFF_SEQNO (10)
#define MESSAGE_HEADER_SZ_SEQNO (1)
#define MESSAGE_HEADER_OFF_SEQACK (11)
#define MESSAGE_HEADER_SZ_SEQACK (1)
#define MESSAGE_HEADER_OFF_CRC (12)
#define MESSAGE_HEADER_SZ_CRC (1)
#define MESSAGE_HEADER_SIZE \
	(MESSAGE_HEADER_SZ_MAGIC + \
	MESSAGE_HEADER_SZ_FLAGS + \
	MESSAGE_HEADER_SZ_PLD_SZ + \
	MESSAGE_HEADER_SZ_PLD_TYPE + \
	MESSAGE_HEADER_SZ_SEQNO + \
	MESSAGE_HEADER_SZ_SEQACK + \
	MESSAGE_HEADER_SZ_CRC)
struct message_header {
	uint8_t magic[4];
	uint16_t msg_flags; /* this message's flags */
	uint16_t payload_sz; /* the size (in bytes) of the payload */
	uint16_t payload_type; /* the payload type */
	uint8_t seqno; /* this message's sequence number (wrapping) */
	uint8_t seq_ack; /* the first unACKed seqno (everything before is considered ACKed) */
	/* old seqnos: (next-128, next-1); wrapping */
	/* new seqnos: (next, next+127); wrapping */
	uint8_t crc8; /* a CRC8 checksum of the serialized bytes, excluding this one */
};

#endif

