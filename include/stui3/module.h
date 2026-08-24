#ifndef STUI3_MODULE_H
#define STUI3_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

#define STUI3_MODULE_SYMBOL_LENGTH 32
#define STUI3_MODULE_NAME_LENGTH 32

#include <stdint.h>
#include <stddef.h>

typedef char stui3_module_symbol[STUI3_MODULE_SYMBOL_LENGTH];
typedef uint32_t stui3_module_abi;

struct stui3_module_description {
	uint8_t magic[4]; /* 0x9A 0x87 0x6E 0x95 */
	stui3_module_abi minimum_abi;
	stui3_module_abi deprecation_abi;
	char name[STUI3_MODULE_NAME_LENGTH];
	char *description;
	stui3_module_symbol *symbols;
	void **pointers;
	size_t num_exports;
	void (*load_hook)();
	void (*unload_hook)();
};

/* struct stui3_module_description stui3_module; */
/* or */
/* struct stui3_module_description THIS_MODULE_NAME_stui3_module; */

#if !defined(COMPILE_MODULE_BUILTIN) && !defined(COMPILE_MODULE_LOADABLE)
	#define COMPILE_MODULE_BUILTIN 0
	#define COMPILE_MODULE_LOADABLE 1
#elif !defined(COMPILE_MODULE_BUILTIN)
	#if COMPILE_MODULE_LOADABLE
		#define COMPILE_MODULE_BUILTIN 0
	#else
		#define COMPILE_MODULE_BUILTIN 1
	#endif
#elif !defined(COMPILE_MODULE_LOADABLE)
	#if COMPILE_MODULE_BUILTIN
		#define COMPILE_MODULE_LOADABLE 0
	#else
		#define COMPILE_MODULE_LOADABLE 1
	#endif
#endif
#if (COMPILE_MODULE_BUILTIN) && (COMPILE_MODULE_LOADABLE)
	#define COMPILE_MODULE_BUILTIN 0
#elif !(COMPILE_MODULE_BUILTIN) && !(COMPILE_MODULE_LOADABLE)
	#define COMPILE_MODULE_LOADABLE 1
#endif

#if COMPILE_MODULE_LOADABLE
#define STUI3_MODULE_NAME(NAME) NAME
#else
#ifndef COMPILE_MODULE_NAME
#error Expected COMPILE_MODULE_NAME as a name.
#endif
#define __CONCAT3(A,B,C) A##B##C
#define CONCAT3(A,B,C) __CONCAT3(A,B,C)
#define STUI3_MODULE_NAME(NAME) CONCAT3(COMPILE_MODULE_NAME,_,NAME)
#endif

#ifdef __cplusplus
};
#endif

#endif

