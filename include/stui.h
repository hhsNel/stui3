#ifndef STUI3_H
#define STUI3_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* $e (size_t) (stui_handle) $F */
typedef size_t stui_handle;
/* $e (uint32_t) (stui_result) $m (stores whether or not the operation was successful ) $F */
typedef uint32_t stui_result;

/* $c */

/* $d (STUI_IHANDLE) (an invalid `stui_handle`) $F */
#define STUI_IHANDLE -1

/* $d (STUI_OK) (a `stui_result` indicating no errors) $F */
#define STUI_OK 0
/* $d (STUI_EOTHER) (a `stui_result` indicating an error not covered by other macros) $F */
#define STUI_EOTHER 1
/* $d (STUI_ENSPRT) (a `stui_result` indicating an unsupported operation) $F */
#define STUI_ENSPRT 2
/* $d (STUI_ENOENT) (a `stui_result` indicating an entry not being found) $F */
#define STUI_ENOENT 3
/* $d (STUI_EUPERR) (a `stui_result` indicating an upstream error) $F */
#define STUI_EUPERR 4
/* $d (STUI_ECRPTD) (a `stui_result` indicating corrupted data) $F */
#define STUI_ECRPTD 5
/* $d (STUI_EOORDR) (a `stui_result` indicating out-of-order operations) $F */
#define STUI_EOORDR 6
/* $d (STUI_EINVAL) (a `stui_result` indicating an invalid argument) $F */
#define STUI_EINVAL 7

/* $M (STUI_IS_OK) (returns whether or not the argument is `STUI_OK`) $F */
#define STUI_IS_OK(EC) ((EC) == (STUI_OK))

/* $d (STUI_NAME_REQUEST_SIZE) (the maximum length of the `name_request` string in `stui_config`) $F */
#define STUI_NAME_REQUEST_SIZE 32

/* $d (STUI_MEM_SHM) (flag representing SHM memory capabilities) $F */
#define STUI_MEM_SHM 1
/* $d (STUI_MEM_LOCAL) (flag representing LOCAL memory capabilities) $F */
#define STUI_MEM_LOCAL 2

/* $c */

/* $m ($(Struct$) `stui_config`) $F */
/* $T (3) $h (type) $h (name) $h (description) */
typedef struct {
	/* $t (`uint32_t`) $t (`agfx_flags`) $t (an AGFX flags bitmask) */
	uint32_t agfx_flags;
	/* $t (`uint32_t`) $t (`input_flags`) $t (an input flags bitmask) */
	uint32_t input_flags;
	/* $t (`uint32_t`) $t (`mem_flags`) $t (a memory flags bitmask) */
	uint32_t mem_flags;
	/* $t (`uint32_t`) $t (`op_flags`) $t (an operaions flags bitmask) */
	uint32_t op_flags;

	/* $t (`char [STUI_NAME_REQUEST_SIZE]`) $t (`name_request`) $t (the requested name for the STUI3 instance) */
	char name_request[STUI_NAME_REQUEST_SIZE];
} stui_config;
/* $m () */

/* $m ($(Enum$) `stui_element_type`) $F */
/* $T (1) $h (value) */
typedef enum {
	/* $t (`STUI_ELEMENT_CONTAINER`) */
	STUI_ELEMENT_CONTAINER,
	/* $t (`STUI_ELEMENT_LABEL`) */
	STUI_ELEMENT_LABEL,
} stui_element_type;
/* $m () */

/* $c */

/* $f (stui_result) (stui_init) */
/* $a (stui_config const*) (conf) (If empty, will be populated with supported flags. Otherwise, will be used to initialize STUI3) */
stui_result
stui_init(stui_config const* conf);
/* $f (void) (stui_exit) */
void
stui_exit(void);

/* $f (stui_result) (stui_main_window) */
/* $a (stui_handle *) (out) (Set to the main window handle for the current instance) */
stui_result
stui_main_window(stui_handle *out);

/* $f (stui_result) (stui_container_add) */
/* $a (stui_handle const) (container) (The container to which the element is moved) */
/* $a (stui_handle const) (element) (The element moved to the container) */
stui_result
stui_container_add(stui_handle const container, stui_handle const element);
/* $f (stui_result) (stui_container_rm) */
/* $a (stui_handle const) (container) (The container from which the element is removed) */
/* $a (stui_handle const) (element) (The element removed from the container) */
stui_result
stui_container_rm(stui_handle const container, stui_handle const element);

/* $f (stui_result) (stui_element_create) */
/* $a (stui_element_type const) (type) (The type of element to create) */
/* $a (...) ($(va list$)) (The arguments for element creation) */
stui_result
stui_element_create(stui_element_type const type, ...);

#ifdef __cplusplus
};
#endif

#endif

