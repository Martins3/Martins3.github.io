#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stddef.h>
#include "sysfs.h"

static void sysfs_write_to_path(const char *path, const long m)
{
	FILE *f;
	f = fopen(path, "w");
	if (!f) {
		fprintf(stderr, "failed to open : %s\n", path);
		exit(1);
	}
	printf("%s %ld\n", path, m);
	fprintf(f, "%ld", m);
	fflush(f);
	fclose(f);
}

static char *get_test_name(char *buf)
{
	ssize_t len = readlink("/proc/self/exe", buf, PATH_MAX - 1);
	if (len != -1) {
		buf[len] = '\0';

		/* 1. 获取最后一个 "/" */
		char *name = strrchr(buf, '/');
		name = name ? name + 1 : buf;

		/* 2. 去掉 "-user.out" 后缀（如果存在） */
		char *dot = strrchr(name, '-');
		if (dot && strcmp(dot, "-user.out") == 0)
			*dot = '\0';

		printf("[martins3:%s:%d] %s\n", __func__, __LINE__, name);
		return name;
	}
	perror("readlink");
	exit(1);
}

void sysfs_write(const long m)
{
	char path[PATH_MAX];
	char test[PATH_MAX] = { 0 };
	char *test_name;
	int ret;
	test_name = get_test_name(test);
	ret = snprintf(path, sizeof(path), "/sys/kernel/hacking/%s", test_name);
	if (ret == PATH_MAX || ret < 0) {
		printf("snprintf\n");
		exit(1);
	}
	sysfs_write_to_path(path, m);
}

void para_write(int para, const long m)
{
	char path[PATH_MAX];
	snprintf(path, sizeof(path), "%s/para%d",
		 "/sys/module/martins3/parameters", para);
	sysfs_write_to_path(path, m);
}
