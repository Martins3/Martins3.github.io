// helper.h
#ifndef __HELPER_H
#define __HELPER_H

#include <stdio.h>
#include <bpf/bpf.h>

// 打印 log2 直方图（简化版）
static void print_log2_hist(int map_fd, const char *val_name,
			    const char *header)
{
	__u64 key = 0, value;
	__u64 max = 0;
	__u64 values[65] = {};

	// 读取所有桶
	while (!bpf_map_get_next_key(map_fd, &key, &key)) {
		if (bpf_map_lookup_elem(map_fd, &key, &value) == 0) {
			values[key] = value;
			if (key > max)
				max = key;
		}
	}

	// 打印表头
	printf("%-16s: %s\n", "log2(pages)", header);
	for (int i = 0; i <= max; i++) {
		if (values[i] > 0) {
			printf("%-16d: %llu\n", i, values[i]);
		}
	}
}

#endif
