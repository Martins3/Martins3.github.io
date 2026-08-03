#pragma once

#include <unistd.h>

void *map_region(unsigned long pages);
void *map_file(unsigned long pages);

/*
 * 为了让 *-user.c 不被内核的 compile_commands.json 影响，所以参考内核，重新定义
 * 一次 NULL
 */
#undef NULL
#define NULL ((void *)0)

static inline long get_pagesize(void)
{
	return sysconf(_SC_PAGESIZE);
}
