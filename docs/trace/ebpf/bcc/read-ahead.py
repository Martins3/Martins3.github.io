#!/usr/bin/python3
from bcc import BPF
from time import sleep
import sys

# BPF 程序
bpf_text = """
#include <uapi/linux/ptrace.h>

// 定义直方图（对数坐标，适合跨度大的数据）
BPF_HISTOGRAM(lookahead_hist, u64);

// 新内核移除了 do_page_cache_readahead，page_cache_sync_readahead 又被内联无法 kprobe。
// read_pages(struct readahead_control *rac) 是所有文件系统 readahead 提交的公共入口，
// readahead_control._nr_pages 位于偏移 32（内核 7.1 BTF 确认），即本次预读请求的页数。
struct ra_min {
    void *file;
    void *mapping;
    void *ra;
    unsigned long _index;
    unsigned int _nr_pages;
};

int kprobe__read_pages(struct pt_regs *ctx, struct ra_min *rac) {
    u32 nr = 0;
    bpf_probe_read_kernel(&nr, sizeof(nr), &rac->_nr_pages);

    // 记录到直方图
    lookahead_hist.increment(bpf_log2l(nr));

    return 0;
}
"""

try:
    b = BPF(text=bpf_text)
except Exception as e:
    print(f"❌ 编译失败: {e}")
    print("⚠️  可能原因: 函数名不对、内核未导出该符号、或参数获取方式不兼容")
    sys.exit(1)

print("Tracing read_pages() req pages... Hit Ctrl-C to end.")

# 打印直方图表头
print("\n%-20s | %s" % ("lookahead_size (log2)", "COUNT"))
print("%-20s | %s" % ("--------------------", "-----"))

try:
    while True:
        sleep(5)  # 每5秒刷新一次

        # 清屏并重绘（可选）
        # print("\033[2J\033[H")  # 清屏（取消注释启用）

        # 打印当前累计直方图
        b["lookahead_hist"].print_log2_hist("lookahead_size")

        print("=" * 50)

except KeyboardInterrupt:
    print("\nExiting...")
    # 打印最终直方图
    print("\nFinal distribution:")
    b["lookahead_hist"].print_log2_hist("lookahead_size")
