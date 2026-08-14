#!/usr/bin/python3
from bcc import BPF
from time import sleep, strftime, localtime
import signal
import sys
import os

# ========== 配置 ==========
DEBUG = False  # True: 同时输出到终端；False: 静默，只写 shm
SHM_FILE = "/dev/shm/ext4_trace_buffer"
BUFFER_SIZE = 10 * 1024 * 1024  # 10MB 预分配内存文件
# ==========================

# 初始化 shm 文件（预分配空间）
try:
    fd = os.open(SHM_FILE, os.O_CREAT | os.O_RDWR, 0o600)
    os.ftruncate(fd, BUFFER_SIZE)
    if DEBUG:
        print(f"[DEBUG] Pre-allocated {BUFFER_SIZE} bytes at {SHM_FILE}")
except Exception as e:
    print(f"❌ Failed to initialize shm file: {e}", file=sys.stderr)
    sys.exit(1)

current_offset = 0  # 当前写入位置


# BPF 程序
bpf_text = """
#include <uapi/linux/ptrace.h>

struct fname_key {
    char name[32];
};

BPF_HASH(file_count, struct fname_key, u64);

// 新内核（>= 6.x）移除了 ext4_filemap_fault，ext4 走通用的 filemap_fault，
// 所以这里统计所有文件系统的 page fault（按文件名）。
// 不包含 <linux/fs.h>：bcc/clang 在 bpf target 下无法编译新版内核头文件
// （struct filename 的匿名成员 static_assert 失败）。改用以下最小 struct，
// 偏移来自内核 7.1 的 BTF：
//   struct vm_fault.vma @0
//   struct vm_area_struct.vm_file @88
//   struct file.f_path @64
//   struct path.dentry @8
//   struct dentry.d_name @32
//   struct qstr.name @8
struct qstr_min { char pad[8]; const char *name; };
struct path_min { void *mnt; struct dentry *dentry; };
struct dentry_min { char pad[32]; struct qstr_min d_name; };
struct file_min {
    char pad[64];
    struct path_min f_path;
};
struct vm_fault_min { struct vm_area_struct *vma; };
struct vma_min { char pad[88]; struct file *vm_file; };

int kprobe__filemap_fault(struct pt_regs *ctx, struct vm_fault_min *vmf) {
    struct file *file = NULL;
    struct dentry *dentry = NULL;
    struct qstr_min d_name = {};
    struct fname_key key = {};

    bpf_probe_read_kernel(&file, sizeof(file), &((struct vma_min *)vmf->vma)->vm_file);
    if (!file) return 0;

    bpf_probe_read_kernel(&dentry, sizeof(dentry), &((struct file_min *)file)->f_path.dentry);
    if (!dentry) return 0;

    bpf_probe_read_kernel(&d_name, sizeof(d_name), &((struct dentry_min *)dentry)->d_name);

    bpf_probe_read_kernel_str(key.name, sizeof(key.name), d_name.name);

    u64 zero = 0;
    u64 *count = file_count.lookup_or_try_init(&key, &zero);
    if (count) {
        (*count)++;
    }

    return 0;
}
"""

b = BPF(text=bpf_text)
if DEBUG:
    print("Tracing ext4_filemap_fault() by filename... Hit Ctrl-C to end.")


def safe_write_line(line):
    """写入一行到 shm 文件，超出则退出"""
    global current_offset
    line_bytes = line.encode("utf-8") + b"\n"
    line_len = len(line_bytes)

    if current_offset + line_len > BUFFER_SIZE:
        print("❌ Buffer full! Exiting to avoid data corruption.", file=sys.stderr)
        sys.exit(1)

    try:
        os.pwrite(fd, line_bytes, current_offset)
        current_offset += line_len
    except Exception as e:
        print(f"❌ Write error: {e}", file=sys.stderr)
        sys.exit(1)

    if DEBUG:
        print(line)


def signal_handler(sig, frame):
    if DEBUG:
        print(
            f"\n[DEBUG] Received Ctrl-C. Data is in {SHM_FILE} (wrote {current_offset} bytes)."
        )
    else:
        print(f"Data written to {SHM_FILE} ({current_offset} bytes). Exiting.")
    # ✅ 不删除 shm 文件！不复制！你后续自行读取！
    os.close(fd)  # 只关闭 fd，文件保留
    sys.exit(0)


signal.signal(signal.SIGINT, signal_handler)


# 主循环
try:
    # 写入表头（只写一次）
    safe_write_line("Timestamp,Filename,Count")

    while True:
        sleep(1)
        timestamp = strftime("%Y-%m-%d %H:%M:%S", localtime())

        counts = b["file_count"]
        entries = []
        total = 0

        for k, v in counts.items():
            fname = k.name.decode("utf-8", "replace").rstrip("\x00")
            count = v.value
            entries.append((fname, count))
            total += count

        entries.sort(key=lambda x: x[1], reverse=True)

        for fname, count in entries:
            safe_write_line(f"{timestamp},{fname},{count}")

        safe_write_line(f"{timestamp},<TOTAL>,{total}")

        counts.clear()

except KeyboardInterrupt:
    pass
