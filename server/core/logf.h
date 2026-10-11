#ifndef SERVER_CORE_LOGF_H
#define SERVER_CORE_LOGF_H

#include "util.h"

void logf_stdout_f(char *fmt, ...);
void logf_stderr_f(char *fmt, ...);

#define logf_stdout(...) logf_stdout_f(__FILE__ " @ " STR(__LINE__) ": " __VA_ARGS__)

#define logf_stderr(...) logf_stderr_f(__FILE__ " @ " STR(__LINE__) ": " __VA_ARGS__)

#endif

