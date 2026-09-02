#ifndef PROTOCOL_COMM_H
#define PROTOCOL_COMM_H

#include "protocol.h"

/* < 0 means error, 0 means success */
int send_client_handshake(int const sock_fd);
/* < 0 means error, 0 means success, > 0 means incompatible */
int recv_server_handshake(int const sock_fd, struct server_handshake *const handshake);
int send_msg_header(int const sock_fd, struct message_header const head);

#endif

