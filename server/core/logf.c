#include "logf.h"

#include "symbols/symbol-table.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

void logf_stdout(char *fmt, ...) {
	va_list vas;
	FILE **sym_stdout_file;
	int *sym_stdout_fd;
	char *sym_stdout_mode;
	int dup_stdout_fd;
	FILE *stdout_file;

	va_start(vas, fmt);

	sym_stdout_file = lookup_symbol("stdout_file");
	if(sym_stdout_file) {
		vfprintf(*sym_stdout_file, fmt, vas);
		return;
	}

	sym_stdout_fd = lookup_symbol("stdout_fd");
	if(sym_stdout_fd && *sym_stdout_fd >= 0) {
		dup_stdout_fd = dup(*sym_stdout_fd);
		if(dup_stdout_fd > 0) {
			sym_stdout_mode = lookup_symbol("stdout_mode");
			if(! sym_stdout_mode) sym_stdout_mode = "w";
			stdout_file = fdopen(dup_stdout_fd, sym_stdout_mode);

			vfprintf(stdout_file, fmt, vas);

			fclose(stdout_file);
			return;
		}
	}

	vfprintf(stdout, fmt, vas);

	va_end(vas);
}

void logf_stderr(char *fmt, ...) {
	va_list vas;
	FILE **sym_stderr_file;
	int *sym_stderr_fd;
	char *sym_stderr_mode;
	int dup_stderr_fd;
	FILE *stderr_file;

	va_start(vas, fmt);

	sym_stderr_file = lookup_symbol("stderr_file");
	if(sym_stderr_file) {
		vfprintf(*sym_stderr_file, fmt, vas);
		return;
	}

	sym_stderr_fd = lookup_symbol("stderr_fd");
	if(sym_stderr_fd && *sym_stderr_fd >= 0) {
		dup_stderr_fd = dup(*sym_stderr_fd);
		if(dup_stderr_fd > 0) {
			sym_stderr_mode = lookup_symbol("stderr_mode");
			if(! sym_stderr_mode) sym_stderr_mode = "w";
			stderr_file = fdopen(dup_stderr_fd, sym_stderr_mode);

			vfprintf(stderr_file, fmt, vas);

			fclose(stderr_file);
			return;
		}
	}

	vfprintf(stderr, fmt, vas);

	va_end(vas);
}

