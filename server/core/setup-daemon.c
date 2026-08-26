#include "setup-daemon.h"

#include <unistd.h>
#include <signal.h>
#include <sys/stat.h>

void become_daemon() {
	pid_t pid;
	int fd;

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
		close(fd);
	}
}

