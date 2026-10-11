#ifndef DYNLOG_FD_ALL_H
#define DYNLOG_FD_ALL_H

#include "stui3/module.h"

int STUI3_MODULE_NAME(write_all)(int fd, void const *buf, size_t sz);
int STUI3_MODULE_NAME(read_expected)(int fd, void *buf, size_t sz);

#endif

