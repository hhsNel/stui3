#ifndef COMMON_SOCK_ALL_H
#define COMMON_SOCK_ALL_H

#include <stddef.h>

/* < 0 means error, 0 means success */
int write_all(int fd, void const *buf, size_t sz);
/* < 0 means error, 0 means success */
int read_expected(int fd, void *buf, size_t sz);

#endif

