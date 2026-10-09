#include "core/setup-daemon.h"
#include "symbols/symbol-table.h"
#include "core/logf.h"
#include "serve/handle-client.h"
#include "util.h"
#include "stui3.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

#define NUM_INCOMING_SOCKETS 3

static int run_server(int *fds, int *connected_fds, unsigned int num_static_fds, sigset_t *orig_mask);
static void handle_term_sig(int sig);
static void connect_sock(struct pollfd **pfds, unsigned int *num_pfds, unsigned int *cap_pfds, struct client_context **ctxs, unsigned int *num_ctxs, unsigned int *cap_ctxs, unsigned int fd_id);

static volatile sig_atomic_t running = 1;

char *unix_sock_path = NULL;
char *tcp_sock_port = NULL;
char *udp_sock_port = NULL;

int main(int argc, char **argv) {
	unsigned int i;
	int ret;
	int unix_socket_fd;
	int tcp_socket_fd;
	int udp_socket_fd;
	struct sockaddr_un unix_socket_address;
	struct sockaddr_in ip_socket_address;
	struct sigaction sa;
	sigset_t block_mask, orig_mask;
	int valid_sockets[NUM_INCOMING_SOCKETS];
	int connected_sockets[NUM_INCOMING_SOCKETS];
	unsigned int num_valid_sockets;

	if((ret = init_symbols()) < 0) exit(ret);

	for(i = 1; i < (unsigned int)argc; ++i) {
#define ARG(STR) (strcmp(argv[i], STR) == 0)
#define ARG_CHECK do { if(i + 1 >= (unsigned int)argc) {fprintf(stderr, "Expected another argument after %s\n", argv[i]); ret = 1; goto shutdown_symbols;} } while(0);
		if(argv[i][0] != '-') {
			fprintf(stderr, "Expected a flag, got: %s\n", argv[i]);
			ret = 1;
			goto shutdown_symbols;
		} else if(ARG("-v") || ARG("--version")) {
			printf("stui3 server\nABI version: %d\n", (int)ABI_VERSION);
			ret = 0;
			goto shutdown_symbols;
		} else if(ARG("-h") || ARG("--help")) {
			printf("%s [options...]\n", argv[0]);
			printf("Options:\n");
			printf("\t%-48s %s\n", "-v | --version", "Display version information");
			printf("\t%-48s %s\n", "-h | --help", "Display this information");
			printf("\t%-48s %s\n", "-U <PATH> | -usock <PATH> | --unix-socket <PATH>", "Set the socket path to <PATH>");
			printf("\t%-48s %s\n", "-t <PORT> | -tcp <PORT> | --tcp-port <PORT>", "Set the TCP port to <PATH>. Disable TCP if 0");
			printf("\t%-48s %s\n", "-u <PORT> | -udp <PORT> | --udp-port <PORT>", "Set the UDP port to <PATH>. Disable TCP if 0");
			ret = 0;
			goto shutdown_symbols;
		} else if(ARG("-U") || ARG("-usock") || ARG("--unix-socket")) {
			ARG_CHECK;
			unix_sock_path = argv[++i];
		} else if(ARG("-t") || ARG("-tcp") || ARG("--tcp-port")) {
			ARG_CHECK;
			tcp_sock_port = argv[++i];
		} else if(ARG("-u") || ARG("-udp") || ARG("--udp-port")) {
			ARG_CHECK;
			udp_sock_port = argv[++i];
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			ret = 1;
			goto shutdown_symbols;
		}
#undef ARG
	}

	if(! unix_sock_path) unix_sock_path = "/tmp/stui3.sock";
	if(! tcp_sock_port) tcp_sock_port = STR(PROTOCOL_TCP_PORT);
	if(! udp_sock_port) udp_sock_port = STR(PROTOCOL_UDP_PORT);

	logf_stdout("stui3 server starting up...\n");
	logf_stdout("The socket path is: %s\n", unix_sock_path);
	logf_stdout("The TCP port is: %s\n", tcp_sock_port);
	logf_stdout("The UDP port is: %s\n", udp_sock_port);

	sa.sa_handler = handle_term_sig;
	sa.sa_flags = 0;
	if(sigemptyset(&sa.sa_mask) < 0) {
		logf_stderr("Could not sigemptyset sa.sa_mask: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	if(sigaction(SIGTERM, &sa, NULL) < 0) {
		logf_stderr("Could not sigaction SIGTERM: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	if(sigaction(SIGINT, &sa, NULL) < 0) {
		logf_stderr("Could not sigaction SIGINT: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	logf_stdout("Signal handling started successfully\n");
	if(sigemptyset(&block_mask) < 0) {
		logf_stderr("Could not sigemptyset block_mask: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	if(sigaddset(&block_mask, SIGTERM) < 0) {
		logf_stderr("Could not sigaddset SIGTERM: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	if(sigaddset(&block_mask, SIGINT) < 0) {
		logf_stderr("Could not sigaddset SIGINT: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	if(sigprocmask(SIG_BLOCK, &block_mask, &orig_mask) != 0) {
		logf_stderr("Could not sigprocmask: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	logf_stdout("Signals ignored apart from ppoll\n");

	if(access(unix_sock_path, F_OK) == 0) {
		logf_stderr("Socket path already exists: %s\n", unix_sock_path);
		ret = 1;
		goto shutdown_symbols;
	}

	unix_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(unix_socket_fd < 0) {
		logf_stderr("Could not open a unix socket: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_symbols;
	}
	logf_stdout("Opened a unix socket (fd: %d)\n", unix_socket_fd);

	memset(&unix_socket_address, 0, sizeof(struct sockaddr_un));
	unix_socket_address.sun_family = AF_UNIX;
	strncpy(unix_socket_address.sun_path, unix_sock_path, sizeof(unix_socket_address.sun_path) - 1);
	if(bind(unix_socket_fd, (struct sockaddr *)&unix_socket_address, sizeof(unix_socket_address)) < 0) {
		logf_stderr("Could not bind: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		ret = 1;
		goto shutdown_socket;
	}
	logf_stdout("Bound unix socket to path %s\n", unix_sock_path);
	valid_sockets[0] = unix_socket_fd;
	connected_sockets[0] = 1;
	num_valid_sockets = 1;

	ret = atoi(tcp_sock_port);
	if(ret) {
		tcp_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
		if(tcp_socket_fd < 0) {
			logf_stderr("Could not open a TCP socket: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
			ret = 1;
			goto shutdown_symbols;
		}
		logf_stdout("Opened a TCP socket (fd: %d)\n", tcp_socket_fd);

		memset(&ip_socket_address, 0, sizeof(struct sockaddr_in));
		ip_socket_address.sin_family = AF_INET;
		ip_socket_address.sin_port = htons(ret);
		ip_socket_address.sin_addr.s_addr = INADDR_ANY;
		if(bind(tcp_socket_fd, (struct sockaddr *)&ip_socket_address, sizeof(ip_socket_address)) < 0) {
			logf_stderr("Could not bind: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
			ret = 1;
			goto shutdown_socket;
		}
		logf_stdout("Bound TCP socket to port %d\n", (int)ret);
		valid_sockets[num_valid_sockets] = tcp_socket_fd;
		connected_sockets[num_valid_sockets++] = 1;
	} else {
		tcp_socket_fd = -1;
		logf_stdout("Not using TCP socket (port: %s)\n", tcp_sock_port);
	}

	ret = atoi(udp_sock_port);
	if(ret) {
		udp_socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
		if(udp_socket_fd < 0) {
			logf_stderr("Could not open a UDP socket: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
			ret = 1;
			goto shutdown_symbols;
		}
		logf_stdout("Opened a UDP socket (fd: %d)\n", udp_socket_fd);

		memset(&ip_socket_address, 0, sizeof(struct sockaddr_in));
		ip_socket_address.sin_family = AF_INET;
		ip_socket_address.sin_port = htons(ret);
		ip_socket_address.sin_addr.s_addr = INADDR_ANY;
		if(bind(udp_socket_fd, (struct sockaddr *)&ip_socket_address, sizeof(ip_socket_address)) < 0) {
			logf_stderr("Could not bind: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
			ret = 1;
			goto shutdown_socket;
		}
		logf_stdout("Bound UDP socket to port %d\n", (int)ret);
		valid_sockets[num_valid_sockets] = udp_socket_fd;
		connected_sockets[num_valid_sockets++] = 0;
	} else {
		udp_socket_fd = -1;
		logf_stdout("Not using UDP socket (port: %s)\n", udp_sock_port);
	}

	logf_stdout("There are %u valid sockets\n", num_valid_sockets);

	for(i = 0; i < num_valid_sockets; ++i) {
		if(connected_sockets[i]) {
			if(listen(valid_sockets[i], SOMAXCONN) < 0) {
				logf_stderr("Could not listen to socket %u: %s (%s)\n", i, strerrordesc_np(errno), strerrorname_np(errno));
				ret = 1;
				goto shutdown_socket;
			}
		}
	}

	for(i = 0; i < num_valid_sockets; ++i) {
		ret = fcntl(valid_sockets[i], F_GETFL, 0);
		if(ret < 0) {
			logf_stderr("Could not fnctl F_GETFL: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
			ret = 1;
			goto shutdown_socket;
		}
		ret |= O_NONBLOCK;
		if(fcntl(valid_sockets[i], F_SETFL, ret) < 0) {
			logf_stderr("Could not fnctl F_SETFL: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
			ret = 1;
			goto shutdown_socket;
		}
	}

	ret = run_server(valid_sockets, connected_sockets, num_valid_sockets, &orig_mask);

shutdown_socket:
	logf_stdout("Shutting down the socket\n");
	unlink(unix_sock_path);
	close(unix_socket_fd);

shutdown_symbols:
	logf_stdout("Shutting down symbol resolution\n");
	shutdown_symbols();
	return ret;
}

static int
run_server(int *fds, int *connected_fds, unsigned int num_static_fds, sigset_t *orig_mask) {
	struct pollfd *pfds;
	unsigned int num_pfds, cap_pfds;
	unsigned int i;
	struct client_context *ctxs;
	unsigned int num_ctxs, cap_ctxs;
	int ret;

	cap_pfds = num_static_fds + 16;
	pfds = malloc(cap_pfds * sizeof(struct pollfd));
	if(! pfds) {
		logf_stderr("Could not malloc pollfds: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		return -STUI3_EUPSTM;
	}
	
	cap_ctxs = 16;
	ctxs = malloc(cap_ctxs * sizeof(struct client_context));
	if(! ctxs) {
		logf_stderr("Could not malloc ctxs: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		return -STUI3_EUPSTM;
	}
	num_ctxs = 0;

	for(i = 0; i < num_static_fds; ++i) {
		pfds[i].fd = fds[i];
		pfds[i].events = POLLIN;
	}
	num_pfds = num_static_fds;

	while(running) {
		if(ppoll(pfds, num_pfds, NULL, orig_mask) > 0) {
			for(i = num_static_fds; i < num_pfds; ++i) {
				if(pfds[i].revents & (POLLIN | POLLOUT)) {
					/* handle client */
					ret = handle_client(&ctxs[i - num_static_fds]);
#define CLEANUP do { \
		close(pfds[i].fd); \
		memcpy(&pfds[i],   &pfds[num_pfds-num_static_fds], sizeof(struct pollfd)); \
		memcpy(&ctxs[i-num_static_fds], &ctxs[num_ctxs], sizeof(struct client_context)); \
		--num_pfds; \
		--num_ctxs; \
		--i; \
		continue; \
	} while(0);
					if(ret < 0) {
						logf_stderr("Error while handling client: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
						CLEANUP;
					}
					if(ret == 0) {
						CLEANUP;
					}
				}
			}
			for(i = 0; i < num_static_fds; ++i) {
				/* TODO: connected fds */
				(void)connected_fds;
				connect_sock(&pfds, &num_pfds, &cap_pfds, &ctxs, &num_ctxs, &cap_ctxs, i);
			}
		}
	}

	free(pfds);
	free(ctxs);
	return 0;
}

static void
handle_term_sig(int sig) {
	(void)sig;
	running = 0;
}

static void
connect_sock(struct pollfd **pfds, unsigned int *num_pfds, unsigned int *cap_pfds, struct client_context **ctxs, unsigned int *num_ctxs, unsigned int *cap_ctxs, unsigned int fd_id) {
	struct pollfd *new_pfds;
	struct client_context *new_ctxs;
	int client_fd;
	int ret;

	if((*pfds)[fd_id].revents & POLLIN) {
		/* accept socket */
		client_fd = accept((*pfds)[fd_id].fd, NULL, NULL);
		if(client_fd >= 0) {
			if(*num_pfds == *cap_pfds) {
				*cap_pfds *= 2;
				new_pfds = realloc(*pfds, *cap_pfds * sizeof(struct pollfd));
				if(! new_pfds) {
					logf_stderr("Could not realloc pollfds: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
					*cap_pfds /= 2;
					close(client_fd);
					return;
				}
				*pfds = new_pfds;
			}
			if(*num_ctxs == *cap_ctxs) {
				*cap_ctxs *= 2;
				new_ctxs = realloc(*ctxs, *cap_ctxs * sizeof(struct client_context));
				if(! new_ctxs) {
					logf_stderr("Could not realloc ctxs: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
					*cap_ctxs /= 2;
					close(client_fd);
					return;
				}
				*ctxs = new_ctxs;
			}
			(*pfds)[*num_pfds].fd = client_fd;
			if((ret = fcntl(client_fd, F_GETFL, 0)) >= 0) {
				if(fcntl(client_fd, F_SETFL, ret | O_NONBLOCK) >= 0) {
					(*pfds)[*num_pfds].events = POLLIN | POLLOUT;
					++*num_pfds;
					init_client_context(client_fd, &(*ctxs)[*num_ctxs]);
					++*num_ctxs;
				} else {
					logf_stderr("Could not fcntl F_SETFL client_fd: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
					close(client_fd);
				}
			} else {
				logf_stderr("Could not fcntl F_GETFL client_fd: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
				close(client_fd);
			}
		} else {
			logf_stderr("Could not accept client_fd: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		}
	}
}

