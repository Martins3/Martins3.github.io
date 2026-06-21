#ifndef MARTINS3_MM_LIB_H
#define MARTINS3_MM_LIB_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

static inline void error(const char *msg)
{
	perror(msg);
	exit(1);
}

static inline long get_page_size(void)
{
	long ret = sysconf(_SC_PAGESIZE);
	if (ret == -1) {
		perror("sysconf(_SC_PAGESIZE)");
		exit(1);
	}
	return ret;
}

#endif
