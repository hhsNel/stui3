#ifndef UTIL_H
#define UTIL_H

#define STUI3_ENOENT (1)
#define STUI3_EUPSTM (2)
#define STUI3_EIACTN (3)
#define STUI3_EIDATA (4)

#define _STR(X) #X
#define STR(X) _STR(X)

#define _EVAL(X) X
#define EVAL(X) _EVAL(X)

#define MAX(A, B) (((A) > (B)) ? (A) : (B))
#define MIN(A, B) (((A) < (B)) ? (A) : (B))

#endif

