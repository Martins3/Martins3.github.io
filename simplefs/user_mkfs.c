// SimpleFS mkfs 工具
// 重构版本：使用 mkfs_common 库

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <errno.h>

#include "mkfs_common.h"

// 获取文件或块设备大小
static int get_device_size(int fd, struct stat *st, size_t *size_out)
{
    if (!st || !size_out)
        return -EINVAL;
    
    // 普通文件
    if (S_ISREG(st->st_mode)) {
        *size_out = st->st_size;
        return 0;
    }
    
    // 块设备
    if (S_ISBLK(st->st_mode)) {
        long long blk_size = 0;
        if (ioctl(fd, BLKGETSIZE64, &blk_size) < 0)
            return -errno;
        *size_out = blk_size;
        return 0;
    }
    
    return -ENOTSUP;  // 不支持的文件类型
}

static int detect_existing_filesystem(int fd, const char *path, char *type_buf,
                                      size_t type_buf_len)
{
    struct simplefs_superblock sb;
    int pipefd[2];
    pid_t pid;
    ssize_t nread;
    int status;

    if (fd < 0 || !path || !type_buf || type_buf_len < 2)
        return -EINVAL;

    type_buf[0] = '\0';

    if (pread(fd, &sb, sizeof(sb), 0) == sizeof(sb) &&
        le32toh(sb.info.magic) == SIMPLEFS_MAGIC) {
        strncpy(type_buf, "simplefs", type_buf_len - 1);
        type_buf[type_buf_len - 1] = '\0';
        return 1;
    }

    if (pipe(pipefd) < 0)
        return -errno;

    pid = fork();
    if (pid < 0) {
        int err = -errno;
        close(pipefd[0]);
        close(pipefd[1]);
        return err;
    }

    if (pid == 0) {
        close(pipefd[0]);
        if (dup2(pipefd[1], STDOUT_FILENO) < 0)
            _exit(127);
        close(pipefd[1]);
        execlp("blkid", "blkid", "-p", "-s", "TYPE", "-o", "value", path,
               (char *)NULL);
        _exit(127);
    }

    close(pipefd[1]);
    nread = read(pipefd[0], type_buf, type_buf_len - 1);
    if (nread < 0)
        nread = 0;
    type_buf[nread] = '\0';
    close(pipefd[0]);

    if (waitpid(pid, &status, 0) < 0)
        return -errno;

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        return 0;

    while (nread > 0 &&
           (type_buf[nread - 1] == '\n' || type_buf[nread - 1] == '\r' ||
            type_buf[nread - 1] == ' ' || type_buf[nread - 1] == '\t')) {
        type_buf[--nread] = '\0';
    }

    if (!type_buf[0])
        return 0;

    return 1;
}

// 打印 superblock 信息
static void print_superblock_info(const struct simplefs_sb_info *info)
{
    printf("Superblock:\n");
    printf("  magic:           0x%08x\n", le32toh(info->magic));
    printf("  nr_blocks:       %u\n", le32toh(info->nr_blocks));
    printf("  nr_inodes:       %u\n", le32toh(info->nr_inodes));
    printf("  nr_istore_blocks: %u\n", le32toh(info->nr_istore_blocks));
    printf("  nr_ifree_blocks:  %u\n", le32toh(info->nr_ifree_blocks));
    printf("  nr_bfree_blocks:  %u\n", le32toh(info->nr_bfree_blocks));
    printf("  nr_free_inodes:  %u\n", le32toh(info->nr_free_inodes));
    printf("  nr_free_blocks:  %u\n", le32toh(info->nr_free_blocks));
}

// 打印布局信息
static void print_layout_info(const struct mkfs_layout *layout)
{
    printf("\nLayout:\n");
    printf("  Total blocks:    %u (%.2f MB)\n",
           layout->nr_blocks,
           (double)layout->nr_blocks * SIMPLEFS_BLOCK_SIZE / (1024 * 1024));
    printf("  Inode store:     %u blocks\n", layout->nr_istore_blocks);
    printf("  Inode bitmap:    %u blocks\n", layout->nr_ifree_blocks);
    printf("  Block bitmap:    %u blocks\n", layout->nr_bfree_blocks);
    printf("  Data blocks:     %u blocks (%.2f MB)\n",
           layout->nr_data_blocks,
           (double)layout->nr_data_blocks * SIMPLEFS_BLOCK_SIZE / (1024 * 1024));
    if (layout->nr_journal_blocks > 0) {
        printf("  Journal:         %u blocks (%.2f MB)\n",
               layout->nr_journal_blocks,
               (double)layout->nr_journal_blocks * SIMPLEFS_BLOCK_SIZE / (1024 * 1024));
        printf("  Journal start:   block %u\n", layout->journal_start_block);
    } else {
        printf("  Journal:         disabled (filesystem too small)\n");
    }
    printf("  First data block: %u\n", layout->first_data_block);
}

int main(int argc, char **argv)
{
    int verbose = 0;
    int force = 0;
    const char *path = NULL;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-f") == 0 ||
                   strcmp(argv[i], "--force") == 0) {
            force = 1;
        } else if (strcmp(argv[i], "-l") == 0) {
            if (++i >= argc || strcmp(argv[i], "defaults") != 0) {
                fprintf(stderr,
                        "Error: only '-l defaults' is supported\n");
                return EXIT_FAILURE;
            }
        } else if (!path) {
            path = argv[i];
        } else {
            fprintf(stderr,
                    "Usage: %s [-f|--force] [-v] [-l defaults] <disk_image>\n",
                    argv[0]);
            return EXIT_FAILURE;
        }
    }

    // 参数检查
    if (!path) {
        fprintf(stderr,
                "Usage: %s [-f|--force] [-v] [-l defaults] <disk_image>\n",
                argv[0]);
        fprintf(stderr, "\nExample:\n");
        fprintf(stderr, "  %s /dev/loop0        # Format block device\n", argv[0]);
        fprintf(stderr, "  %s fs.img            # Format file image\n", argv[0]);
        return EXIT_FAILURE;
    }

    char foreign_fs[128];
    
    // 打开设备/文件
    int fd = open(path, O_RDWR | O_EXCL);
    if (fd < 0) {
        if (errno == EBUSY) {
            fprintf(stderr, "Error: %s is busy (may be mounted)\n", path);
        } else {
            fprintf(stderr, "Error: cannot open %s: %s\n", path, strerror(errno));
        }
        return EXIT_FAILURE;
    }
    
    // 获取文件状态
    struct stat st;
    if (fstat(fd, &st) < 0) {
        fprintf(stderr, "Error: fstat failed: %s\n", strerror(errno));
        close(fd);
        return EXIT_FAILURE;
    }
    
    // 获取大小
    size_t image_size = 0;
    int ret = get_device_size(fd, &st, &image_size);
    if (ret < 0) {
        fprintf(stderr, "Error: cannot get size of %s: %s\n", 
                path, strerror(-ret));
        close(fd);
        return EXIT_FAILURE;
    }
    
    if (verbose) {
        printf("Device: %s\n", path);
        printf("Size:   %zu bytes (%.2f MB)\n", 
               image_size, (double)image_size / (1024 * 1024));
    }
    
    // 检查最小大小
    if (image_size <= SIMPLEFS_MIN_SIZE) {
        fprintf(stderr, "Error: %s is too small (%zu bytes, min %d bytes)\n",
                path, image_size, SIMPLEFS_MIN_SIZE);
        close(fd);
        return EXIT_FAILURE;
    }

    ret = detect_existing_filesystem(fd, path, foreign_fs,
                                     sizeof(foreign_fs));
    if (ret < 0) {
        fprintf(stderr, "Error: cannot probe %s: %s\n",
                path, strerror(-ret));
        close(fd);
        return EXIT_FAILURE;
    }
    if (ret > 0 && !force) {
        fprintf(stderr,
                "Error: refusing to overwrite existing filesystem signature '%s' on %s\n",
                foreign_fs, path);
        close(fd);
        return EXIT_FAILURE;
    }

    // 创建文件系统
    struct mkfs_result result;
    ret = mkfs_create_fs(fd, image_size, &result);
    
    if (ret < 0) {
        fprintf(stderr, "Error: %s\n", result.error_msg);
        close(fd);
        return EXIT_FAILURE;
    }
    
    // 打印信息
    if (verbose) {
        print_superblock_info(&result.sb_info);
        
        // 重新计算 layout 用于显示
        struct mkfs_layout layout;
        mkfs_calculate_layout(image_size, &layout);
        print_layout_info(&layout);
        
        printf("\nFormat successful!\n");
    } else {
        printf("Created SimpleFS on %s (%u blocks, %u inodes)\n",
               path,
               le32toh(result.sb_info.nr_blocks),
               le32toh(result.sb_info.nr_inodes));
    }
    
    close(fd);
    return EXIT_SUCCESS;
}
