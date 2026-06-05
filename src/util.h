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

#endif

