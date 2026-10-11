#ifndef DYNLOG_PARENT_H
#define DYNLOG_PARENT_H

#include "stui3/module.h"

int STUI3_MODULE_NAME(exec_command)(char const *const in, char *const out, size_t const out_sz);

int STUI3_MODULE_NAME(command)(char const *const in, char *const out, size_t const out_sz);

#endif

