#ifndef UTIL_H
#define UTIL_H

#define _STR(X) #X
#define STR(X) _STR(X)

#if (defined(__clang__))
#define COMPILER_CLANG 1
#define COMPILER_GCC 0
#elif ((defined(__GNUC__)) || (defined(__GNUG__)))
#define COMPILER_CLANG 0
#define COMPILER_GCC 1
#else
#define COMPILER_CLANG 0
#define COMPILER_GCC 0
#endif

#if ((defined(DEBUG)) && ((DEBUG) == 1))
#if ((COMPILER_CLANG) || (COMPILER_GCC))
#define MESSAGE(MSG) _Pragma(STR(message(MSG)))
#else
/* not supported */
#define MESSAGE(MSG)
#endif
#else
/* no debug */
#define MESSAGE(MSG)
#endif

#if ((COMPILER_CLANG) || (COMPILER_GCC))
#define ALIGNOF(T) __alignof__(T)
#else
#include <stddef.h>
#define ALIGNOF(T) offsetof(struct { char c; T member; }, member)
#endif

typedef union {
	char str[8];
	uint64_t num;
} magic_id;

#define TO_MAGIC_ID(STR) ((magic_id){.str={STR[0],STR[1],STR[2],STR[3],STR[4],STR[5],STR[6],STR[7]}})

#endif

