#ifndef LIB_TRANSPORT_CLIENT_INIT_H
#define LIB_TRANSPORT_CLIENT_INIT_H

#include "protocol.h"
#include "transport/comm.h"

/* < 0 means error, 0 means success, > 0 means incompatible */
int client_handshake(int const sock_fd, struct transport_protocol *const tp);

#endif

