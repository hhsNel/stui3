#include "mem/shm.h"

#include "skip.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *result_name(stui_result r)
{
	switch (r) {
	case STUI_OK: return "STUI_OK";
	case STUI_EUPERR: return "STUI_EUPERR";
	case STUI_EOORDR: return "STUI_EOORDR";
	default: return "STUI_UNKNOWN";
	}
}

int main(void)
{
	struct mem_shm_data msd;
	int is_parent = 0;
	char line[512];
	stui_result res;

	SKIP_UNLESS(MEM_BACKEND_SHM);

	setvbuf(stdout, NULL, _IONBF, 0);

	memset(&msd, 0, sizeof msd);
	msd.fd = -1;

	puts("READY");

	while (fgets(line, sizeof line, stdin)) {
		size_t len = strlen(line);
		while (len && (line[len - 1] == '\n' || line[len - 1] == '\r'))
			line[--len] = '\0';

		if (strncmp(line, "init ", 5) == 0) {
			char name[STUI_NAME_REQUEST_SIZE];
			memset(name, 0, sizeof name);
			strncpy(name, line + 5, sizeof name - 1);

			res = mem_shm_init(name, &msd, &is_parent);
			if (res == STUI_OK)
				printf("OK %s\n", is_parent ? "parent" : "child");
			else
				printf("ERR %s\n", result_name(res));

		} else if (strncmp(line, "map ", 4) == 0) {
			size_t sz = (size_t)strtoull(line + 4, NULL, 10);
			res = mem_shm_map(sz, is_parent, &msd);
			if (res == STUI_OK)
				printf("OK\n");
			else
				printf("ERR %s\n", result_name(res));

		} else if (strncmp(line, "remap ", 6) == 0) {
			size_t sz = (size_t)strtoull(line + 6, NULL, 10);
			res = mem_shm_remap(sz, &msd);
			if (res == STUI_OK)
				printf("OK\n");
			else
				printf("ERR %s\n", result_name(res));

		} else if (strcmp(line, "unmap") == 0) {
			res = mem_shm_unmap(is_parent, &msd);
			if (res == STUI_OK)
				printf("OK\n");
			else
				printf("ERR %s\n", result_name(res));

		} else if (strncmp(line, "write ", 6) == 0) {
			size_t offset;
			unsigned int byte;
			if (sscanf(line + 6, "%zu %u", &offset, &byte) != 2) {
				printf("ERR BAD_ARGS\n");
			} else if (!msd.base || offset >= msd.mapped_size) {
				printf("ERR OUT_OF_BOUNDS\n");
			} else {
				((unsigned char *)msd.base)[offset] = (unsigned char)byte;
				printf("OK\n");
			}

		} else if (strncmp(line, "read ", 5) == 0) {
			size_t offset = (size_t)strtoull(line + 5, NULL, 10);
			if (!msd.base || offset >= msd.mapped_size) {
				printf("ERR OUT_OF_BOUNDS\n");
			} else {
				printf("OK %u\n", (unsigned int)((unsigned char *)msd.base)[offset]);
			}

		} else if (strcmp(line, "state") == 0) {
			printf("OK fd=%d base=%d size=%zu\n",
				   msd.fd,
				   msd.base != NULL ? 1 : 0,
				   msd.mapped_size);

		} else if (strcmp(line, "quit") == 0) {
			break;

		} else {
			printf("ERR UNKNOWN_CMD\n");
		}
	}

	return 0;
}

