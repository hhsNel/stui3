#include "setup-daemon.h"

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>

int ctty_fd;

void become_daemon() {
	pid_t pid;
	int fd;
	char ctty_path[L_ctermid];

	if(ctermid(ctty_path)) {
		ctty_fd = open(ctty_path, O_RDWR | O_NOCTTY);
	} else {
		ctty_fd = -1;
	}

	pid = fork();
	if(pid < 0) _exit(1);
	if(pid > 0) _exit(0);

	if(setsid() < 0) _exit(1);

	signal(SIGCHLD, SIG_IGN);
	signal(SIGHUP, SIG_IGN);

	pid = fork();
	if(pid < 0) _exit(1);
	if(pid > 0) _exit(0);

	umask(0);
	chdir("/");

	for(fd = sysconf(_SC_OPEN_MAX); fd >= 0; --fd) {
		if(fd != ctty_fd) {
			close(fd);
		}
	}
}

