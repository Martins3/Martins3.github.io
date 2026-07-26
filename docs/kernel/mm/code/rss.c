/*
 * rss.c - 完整测试 mmap RSS 变化
 * 对比: 普通文件 vs tmpfs 文件, MAP_PRIVATE vs MAP_SHARED
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdint.h>

#define TEST_SIZE (10 * 1024 * 1024)  // 10MB

static void print_status(const char* label) {
    FILE *fp = fopen("/proc/self/status", "r");
    if (!fp) return;

    printf("\n=== %s ===\n", label);
    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "VmRSS:", 6) == 0 ||
            strncmp(line, "RssAnon:", 8) == 0 ||
            strncmp(line, "RssFile:", 8) == 0 ||
            strncmp(line, "RssShmem:", 9) == 0)
            printf("%s", line);
    }
    fclose(fp);
}

static int create_test_file(const char *path) {
    int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);
    if (fd < 0) {
        perror("open");
        return -1;
    }

    char buf[4096];
    memset(buf, 'X', sizeof(buf));
    for (int i = 0; i < TEST_SIZE / sizeof(buf); i++) {
        if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
            perror("write");
            close(fd);
            return -1;
        }
    }
    lseek(fd, 0, SEEK_SET);
    return fd;
}

static void test_regular_private(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 1: Regular File + MAP_PRIVATE + Write                ║\n");
    printf("║  (Ext4/XFS on disk - NOT tmpfs)                           ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    int fd = create_test_file("/var/tmp/test_regular.bin");
    if (fd < 0) return;

    print_status("Initial");

    char *map = mmap(NULL, TEST_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return;
    }

    // 读取
    volatile char dummy = 0;
    for (size_t i = 0; i < TEST_SIZE; i += 4096) dummy += map[i];
    print_status("After read (should be RssFile)");

    // 写入触发 COW
    for (size_t i = 0; i < TEST_SIZE; i += 4096) map[i] = 'Y';
    print_status("After write (COW: RssFile->RssAnon)");

    munmap(map, TEST_SIZE);
    close(fd);
    unlink("/var/tmp/test_regular.bin");
    print_status("After cleanup");
}

static void test_tmpfs_private(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 2: tmpfs File + MAP_PRIVATE + Write                  ║\n");
    printf("║  (/tmp is typically tmpfs)                                ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    int fd = create_test_file("/tmp/test_tmpfs.bin");
    if (fd < 0) return;

    print_status("Initial");

    char *map = mmap(NULL, TEST_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return;
    }

    // 读取
    volatile char dummy = 0;
    for (size_t i = 0; i < TEST_SIZE; i += 4096) dummy += map[i];
    print_status("After read (tmpfs = RssShmem, not RssFile!)");

    // 写入触发 COW
    for (size_t i = 0; i < TEST_SIZE; i += 4096) map[i] = 'Z';
    print_status("After write (COW: RssShmem->RssAnon)");

    munmap(map, TEST_SIZE);
    close(fd);
    unlink("/tmp/test_tmpfs.bin");
    print_status("After cleanup");
}

static void test_regular_shared(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 3: Regular File + MAP_SHARED + Write                 ║\n");
    printf("║  (No COW - writes go directly to file page cache)         ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    int fd = create_test_file("/var/tmp/test_shared.bin");
    if (fd < 0) return;

    print_status("Initial");

    char *map = mmap(NULL, TEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return;
    }

    // 读取
    volatile char dummy = 0;
    for (size_t i = 0; i < TEST_SIZE; i += 4096) dummy += map[i];
    print_status("After read (RssFile)");

    // 写入 - 不触发 COW
    for (size_t i = 0; i < TEST_SIZE; i += 4096) map[i] = 'W';
    print_status("After write (NO COW - still RssFile)");

    munmap(map, TEST_SIZE);
    close(fd);
    unlink("/var/tmp/test_shared.bin");
    print_status("After cleanup");
}

static void test_tmpfs_shared(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 4: tmpfs File + MAP_SHARED + Write                   ║\n");
    printf("║  (Shared mapping of shared memory - stays RssShmem)       ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    int fd = create_test_file("/tmp/test_tmpfs_shared.bin");
    if (fd < 0) return;

    print_status("Initial");

    char *map = mmap(NULL, TEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return;
    }

    // 读取
    volatile char dummy = 0;
    for (size_t i = 0; i < TEST_SIZE; i += 4096) dummy += map[i];
    print_status("After read (RssShmem)");

    // 写入 - 不触发 COW
    for (size_t i = 0; i < TEST_SIZE; i += 4096) map[i] = 'V';
    print_status("After write (NO COW - still RssShmem)");

    munmap(map, TEST_SIZE);
    close(fd);
    unlink("/tmp/test_tmpfs_shared.bin");
    print_status("After cleanup");
}

static void test_anon_shared(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║  TEST 5: Anonymous MAP_SHARED (shm without file)          ║\n");
    printf("║  (MAP_ANONYMOUS | MAP_SHARED)                             ║\n");
    printf("╚════════════════════════════════════════════════════════════╝\n");

    print_status("Initial");

    char *map = mmap(NULL, TEST_SIZE, PROT_READ | PROT_WRITE,
                     MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (map == MAP_FAILED) {
        perror("mmap");
        return;
    }

    print_status("After mmap (not touched)");

    // 访问
    for (size_t i = 0; i < TEST_SIZE; i += 4096) map[i] = 'U';
    print_status("After write (RssShmem - shared anonymous)");

    munmap(map, TEST_SIZE);
    print_status("After cleanup");
}

int main(void) {
    printf("PID: %d\n", getpid());
    printf("Testing RSS behavior with different mmap configurations\n");
    printf("Each test uses %d MB\n", TEST_SIZE / (1024*1024));

    test_regular_private();
    test_tmpfs_private();
    test_regular_shared();
    test_tmpfs_shared();
    test_anon_shared();

    printf("\n");
    printf("╔════════════════════════════════════════════════════════════╗\n");
    printf("║                      SUMMARY                               ║\n");
    printf("╠════════════════════════════════════════════════════════════╣\n");
    printf("║  File Type    │  Map Type   │  After Read  │  After Write ║\n");
    printf("╠═══════════════╪═════════════╪══════════════╪══════════════╣\n");
    printf("║  Regular File │  PRIVATE    │  RssFile     │  RssAnon     ║\n");
    printf("║  Regular File │  SHARED     │  RssFile     │  RssFile     ║\n");
    printf("║  tmpfs File   │  PRIVATE    │  RssShmem    │  RssAnon     ║\n");
    printf("║  tmpfs File   │  SHARED     │  RssShmem    │  RssShmem    ║\n");
    printf("║  Anonymous    │  SHARED     │  N/A         │  RssShmem    ║\n");
    printf("╚═══════════════╧═════════════╧══════════════╧══════════════╝\n");
    printf("\nKey Insight:\n");
    printf("- COW (Copy-On-Write) only triggers with PRIVATE + write\n");
    printf("- tmpfs files are backed by swap, counted as RssShmem\n");
    printf("- MAP_SHARED anonymous = POSIX shared memory\n");

    return 0;
}

