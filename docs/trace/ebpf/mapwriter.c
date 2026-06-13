// SPDX-License-Identifier: BSD-3-Clause
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include "mapwriter.skel.h"

static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
			   va_list args)
{
	return vfprintf(stderr, format, args);
}

int main(int argc, char **argv)
{
	struct mapwriter_bpf *prog;
	int err;

	// Make BPF debug logs go to to stderr
	libbpf_set_print(libbpf_print_fn);

	// Open and log BPF Program
	prog = mapwriter_bpf__open();
	if (!prog) {
		printf("Failed to open BPF skeleton\n");
		return 1;
	}
	err = mapwriter_bpf__load(prog);
	if (err) {
		printf("Failed to load and verify BPF skeleton\n");
		goto cleanup;
	}

	// Create socket and attack bpf program to it
	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets)) {
		printf("failed to create socket pair '%s'\n", strerror(errno));
		goto cleanup;
	}
	// TODO 这个 my_socket_prog 是咋生成的 ?
	// 也就是 mapwriter.bpf.c 可以生成多个 bpf 段
	int prog_fd[1] = { bpf_program__fd(prog->progs.my_socket_prog) };
	if (setsockopt(sockets[1], SOL_SOCKET, SO_ATTACH_BPF, prog_fd,
		       sizeof(prog_fd[0])) < 0) {
		printf("setsockopt '%s'\n", strerror(errno));
		return -1;
	}

	// 通过 sudo cat /sys/kernel/debug/tracing/trace_pipe 可以观察到
	int key = 0;
	int value = 286331153; // "11 11 11 11"
	printf("-----------------------\n");
	printf("Writing data into a Map from user and kernel\n");
	while (1) {
		// Write value into map
		bpf_map_update_elem(bpf_map__fd(prog->maps.my_map), &key,
				    &value, BPF_ANY);

		// https://patchwork.ozlabs.org/project/netdev/patch/20191117172806.2195367-4-andriin@fb.com/
		//
		// #0  array_map_update_elem (map=0xffff88810648d000, key=0xffff888104e143b0, value=0xffff888104e16c30, map_flags=0) at ./kernel/bpf/arraymap.c:349
		// #1  0xffffffff8133c552 in bpf_map_update_value (map=0xffff88810648d000, map_file=0xffff888105c9ad00, key=0xffff888104e143b0, value=0xffff888104e16c30, flags=0) at ./kernel/bpf/syscall.c:203
		// #2  0xffffffff8133f578 in map_update_elem (attr=0xffffc90015617e28, uattr=...) at ./kernel/bpf/syscall.c:1654
		// #3  0xffffffff8133ec2a in __sys_bpf (cmd=2, uattr=..., size=32) at ./kernel/bpf/syscall.c:5698
		// #4  0xffffffff8133da8c in __do_sys_bpf (cmd=-2098864464, uattr=0xffff888104e143b0, size=0)

		void *ptr = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
				 MAP_SHARED, bpf_map__fd(prog->maps.my_map), 0);
		if (ptr == MAP_FAILED)
			printf("[martins3:%s:%d] failed \n", __FUNCTION__,
			       __LINE__);
		else {
			// dr-x------ 2 root root  9 Oct 18 15:03 .
			// dr-xr-xr-x 9 root root  0 Oct 18 15:02 ..
			// lrwx------ 1 root root 64 Oct 18 15:03 0 -> /dev/pts/1
			// lrwx------ 1 root root 64 Oct 18 15:03 1 -> /dev/pts/1
			// lrwx------ 1 root root 64 Oct 18 15:03 2 -> /dev/pts/1
			// lrwx------ 1 root root 64 Oct 18 15:03 3 -> anon_inode:bpf-map
			// lrwx------ 1 root root 64 Oct 18 15:03 4 -> anon_inode:bpf-map
			// lr-x------ 1 root root 64 Oct 18 15:03 5 -> anon_inode:btf
			// lrwx------ 1 root root 64 Oct 18 15:03 6 -> anon_inode:bpf-prog
			// lrwx------ 1 root root 64 Oct 18 15:03 7 -> 'socket:[32028]'
			// lrwx------ 1 root root 64 Oct 18 15:03 8 -> 'socket:[32029]'
			printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__,
			       bpf_map__fd(
				       prog->maps.my_map)); // 输出是 3 ，也就是 map 用的是 anon_inode:bpf-map 就可以了
			/*
			 * #0  array_map_mmap (map=0xffffc90012601f08, vma=0xffff888103a45d90) at ./kernel/bpf/arraymap.c:562
			 * #1  0xffffffff8133b7b1 in bpf_map_mmap (filp=<optimized out>, vma=0xffff888103a45d90) at ./kernel/bpf/syscall.c:965
			 * #2  0xffffffff8144a4d2 in call_mmap (file=0xffff888105c9a600, vma=0xffff888103a45d90) at ./include/linux/fs.h:2130
			 * #3  mmap_region (file=0xffff888105c9a600, addr=140577006505984, len=4096, vm_flags=251, pgoff=0, uf=<optimized out>) at mm/mmap.c:2957
			 * #4  0xffffffff81449b53 in do_mmap (file=<optimized out>, addr=140577006505984, len=<optimized out>, prot=3, flags=<optimized out>, vm_flags=251, pgoff=<optimized out>, populate=0xffffc90017147e78, uf=0xffffc90017147e68) at mm/mmap.c:1468
			 * #5  0xffffffff8141235c in vm_mmap_pgoff (file=<optimized out>, addr=0, len=4096, prot=<optimized out>, flag=1, pgoff=<optimized out>) at mm/util.c:588
			 * #6  0xffffffff8144ab89 in ksys_mmap_pgoff (addr=0, len=4096, prot=3, flags=1, fd=<optimized out>, pgoff=0) at mm/mmap.c:1514
			 * #7  0xffffffff826a830d in do_syscall_x64 (regs=0xffffc90017147f58, nr=9) at arch/x86/entry/common.c:52
			 * #8  do_syscall_64 (regs=0xffffc90017147f58, nr=9) at arch/x86/entry/common.c:83
			 * #9  0xffffffff82800130 in entry_SYSCALL_64 () at /home/martins3/data/linux-build/arch/x86/entry/entry_64.S:121
			 */
			int *m = (int *)ptr;
			printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__,
			       *m);
			*m = 12;
		}
		sleep(1);

		// Write arbitrary data to socket to trigger bpf program
		char buffer[4] = { 'a', 'b', 'c', 'd' };
		size_t socket_n = write(sockets[0], buffer, sizeof(buffer));
		if (socket_n < 0) {
			perror("write");
			return -1;
		}
		if (socket_n != sizeof(buffer)) {
			printf("short write: %zd\n", socket_n);
			return -1;
		}
		fprintf(stderr, ".");
		sleep(1);
	}
cleanup:
	mapwriter_bpf__destroy(prog);
	return -err;
}
