#include "logf.h"

#include "symbols/symbol-table.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>

void logf_stdout_f(char *fmt, ...) {
	va_list vas;
	FILE **sym_stdout_file;
	int *sym_stdout_fd;

	va_start(vas, fmt);

	sym_stdout_file = lookup_symbol("stdout_file");
	if(sym_stdout_file) {
		vfprintf(*sym_stdout_file, fmt, vas);
		va_end(vas);
		fflush(*sym_stdout_file);
		return;
	}

	sym_stdout_fd = lookup_symbol("stdout_fd");
	if(sym_stdout_fd && *sym_stdout_fd >= 0) {
		vdprintf(*sym_stdout_fd, fmt, vas);
		va_end(vas);
		return;
	}

	vfprintf(stdout, fmt, vas);
	fflush(stdout);
	va_end(vas);
}

void logf_stderr_f(char *fmt, ...) {
	va_list vas;
	FILE **sym_stderr_file;
	int *sym_stderr_fd;

	va_start(vas, fmt);

	sym_stderr_file = lookup_symbol("stderr_file");
	if(sym_stderr_file) {
		vfprintf(*sym_stderr_file, fmt, vas);
		va_end(vas);
		fflush(*sym_stderr_file);
		return;
	}

	sym_stderr_fd = lookup_symbol("stderr_fd");
	if(sym_stderr_fd && *sym_stderr_fd >= 0) {
		vdprintf(*sym_stderr_fd, fmt, vas);
		va_end(vas);
		return;
	}

	vfprintf(stderr, fmt, vas);
	fflush(stderr);
	va_end(vas);
}

