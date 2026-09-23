#include "core/setup-daemon.h"
#include "symbols/symbol-table.h"
#include "core/logf.h"
#include "serve/handle-client.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/un.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>

static void run_server(int socket_fd, sigset_t *orig_mask);
static void handle_term_sig(int sig);

static volatile sig_atomic_t running = 1;

int main(int argc, char **argv) {
	unsigned int i;
	int ret;
	int socket_fd;
	struct sockaddr_un socket_address;
	struct sigaction sa;
	sigset_t block_mask, orig_mask;

	char *sock_path = NULL;

	if((ret = init_symbols()) < 0) exit(ret);

	for(i = 1; i < (unsigned int)argc; ++i) {
#define ARG(STR) (strcmp(argv[i], STR) == 0)
#define ARG_CHECK do { if(i + 1 >= (unsigned int)argc) {fprintf(stderr, "Expected another argument after %s\n", argv[i]); exit(1);} } while(0);
		if(argv[i][0] != '-') {
			fprintf(stderr, "Expected a flag, got: %s\n", argv[i]);
			exit(1);
		} else if(ARG("-v") || ARG("--version")) {
			printf("stui3 server\nABI version: %d\n", (int)ABI_VERSION);
			exit(0);
		} else if(ARG("-h") || ARG("--help")) {
			printf("%s [options...]\n", argv[0]);
			printf("Options:\n");
			printf("\t%-48s %s\n", "-v | --version", "Display version information");
			printf("\t%-48s %s\n", "-h | --help", "Display this information");
			printf("\t%-48s %s\n", "-s <PATH> | -sock <PATH> | --sock <PATH>", "Set the socket path to <PATH>");
			exit(0);
		} else if(ARG("-s") || ARG("-sock") || ARG("--sock")) {
			ARG_CHECK;
			sock_path = argv[++i];
		} else {
			fprintf(stderr, "Unknown option: %s\n", argv[i]);
			exit(1);
		}
#undef ARG
	}

	if(! sock_path) sock_path = "/tmp/stui3.sock";

	logf_stdout("stui3 server starting up...\n");
	logf_stdout("The socket path is: %s\n", sock_path);

	sa.sa_handler = handle_term_sig;
	sa.sa_flags = 0;
	if(sigemptyset(&sa.sa_mask) < 0) {
		logf_stderr("Could not sigemptyset sa.sa_mask: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	if(sigaction(SIGTERM, &sa, NULL) < 0) {
		logf_stderr("Could not sigaction SIGTERM: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	if(sigaction(SIGINT, &sa, NULL) < 0) {
		logf_stderr("Could not sigaction SIGINT: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	logf_stdout("Signal handling started successfully\n");
	if(sigemptyset(&block_mask) < 0) {
		logf_stderr("Could not sigemptyset block_mask: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	if(sigaddset(&block_mask, SIGTERM) < 0) {
		logf_stderr("Could not sigaddset SIGTERM: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	if(sigaddset(&block_mask, SIGINT) < 0) {
		logf_stderr("Could not sigaddset SIGINT: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	if(sigprocmask(SIG_BLOCK, &block_mask, &orig_mask) != 0) {
		logf_stderr("Could not sigprocmask: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	logf_stdout("Signals ignored apart from ppoll\n");

	if(access(sock_path, F_OK) == 0) {
		logf_stderr("Socket path already exists: %s", sock_path);
		exit(1);
	}

	socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if(socket_fd < 0) {
		logf_stderr("Could not open a socket: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		exit(1);
	}
	logf_stdout("Opened a socket (fd: %d)\n", socket_fd);

	memset(&socket_address, 0, sizeof(struct sockaddr_un));
	socket_address.sun_family = AF_UNIX;
	strncpy(socket_address.sun_path, sock_path, sizeof(socket_address.sun_path) - 1);
	if(bind(socket_fd, (struct sockaddr *)&socket_address, sizeof(socket_address)) < 0) {
		logf_stderr("Could not bind: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		goto shutdown;
	}
	logf_stdout("Bound socket to path %s\n", sock_path);

	if(listen(socket_fd, SOMAXCONN) < 0) {
		logf_stderr("Could not listen: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		goto shutdown;
	}

	ret = fcntl(socket_fd, F_GETFL, 0);
	if(ret < 0) {
		logf_stderr("Could not fnctl F_GETFL: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		goto shutdown;
	}
	ret |= O_NONBLOCK;
	if(fcntl(socket_fd, F_SETFL, ret) < 0) {
		logf_stderr("Could not fnctl F_SETFL: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		goto shutdown;
	}

	run_server(socket_fd, &orig_mask);

shutdown:
	logf_stdout("Shutting down the socket\n");
	unlink(sock_path);
	close(socket_fd);

	logf_stdout("Shutting down symbol resolution\n");
	shutdown_symbols();
}

static void
run_server(int socket_fd, sigset_t *orig_mask) {
	struct pollfd *pfds, *new_pfds;
	unsigned int num_pfds, cap_pfds;
	int client_fd;
	unsigned int i;
	struct client_context *ctxs, *new_ctxs;
	unsigned int num_ctxs, cap_ctxs;
	int ret;

	cap_pfds = 16;
	pfds = malloc(cap_pfds * sizeof(struct pollfd));
	if(! pfds) {
		logf_stderr("Could not malloc pollfds: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		return;
	}
	
	cap_ctxs = 16;
	ctxs = malloc(cap_ctxs * sizeof(struct client_context));
	if(! ctxs) {
		logf_stderr("Could not malloc ctxs: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
		return;
	}
	num_ctxs = 0;

	pfds[0].fd = socket_fd;
	pfds[0].events = POLLIN;
	num_pfds = 1;

	while(running) {
		if(ppoll(pfds, num_pfds, NULL, orig_mask) > 0) {
			for(i = 1; i < num_pfds; ++i) {
				if(pfds[i].revents & (POLLIN | POLLOUT)) {
					/* handle client */
					ret = handle_client(&ctxs[i - 1]);
#define CLEANUP do { \
		close(pfds[i].fd); \
		memcpy(&pfds[i],   &pfds[num_pfds-1], sizeof(struct pollfd)); \
		memcpy(&ctxs[i-1], &ctxs[num_ctxs-1], sizeof(struct client_context)); \
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
			if(pfds[0].revents & POLLIN) {
				/* accept socket */
				client_fd = accept(socket_fd, NULL, NULL);
				if(client_fd >= 0) {
					if(num_pfds == cap_pfds) {
						cap_pfds *= 2;
						new_pfds = realloc(pfds, cap_pfds * sizeof(struct pollfd));
						if(! new_pfds) {
							logf_stderr("Could not realloc pollfds: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
							cap_pfds /= 2;
							close(client_fd);
							continue;
						}
						pfds = new_pfds;
					}
					if(num_ctxs == cap_ctxs) {
						cap_ctxs *= 2;
						new_ctxs = realloc(ctxs, cap_ctxs * sizeof(struct client_context));
						if(! new_ctxs) {
							logf_stderr("Could not realloc ctxs: %s (%s)\n", strerrordesc_np(errno), strerrorname_np(errno));
							cap_ctxs /= 2;
							close(client_fd);
							continue;
						}
						ctxs = new_ctxs;
					}
					pfds[num_pfds].fd = client_fd;
					if((ret = fcntl(client_fd, F_GETFL, 0)) >= 0) {
						if(fcntl(client_fd, F_SETFL, ret | O_NONBLOCK) >= 0) {
							pfds[num_pfds].events = POLLIN | POLLOUT;
							++num_pfds;
							init_client_context(client_fd, &ctxs[num_ctxs]);
							++num_ctxs;
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
	}

	free(pfds);
	free(ctxs);
}

static void
handle_term_sig(int sig) {
	(void)sig;
	running = 0;
}

