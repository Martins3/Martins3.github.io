#!/usr/bin/python3
from bpfcc import BPF
from time import sleep
import sys

# BPF 程序
bpf_text = """
#include <uapi/linux/ptrace.h>

// 定义直方图（对数坐标，适合跨度大的数据）
BPF_HISTOGRAM(lookahead_hist, u64);

int kprobe__do_page_cache_readahead(struct pt_regs *ctx) {
    // 第5个参数 lookahead_size，在 x86_64 中是 ctx->r8
    u64 lookahead_size = PT_REGS_PARM5(ctx);

    // 记录到直方图
    lookahead_hist.increment(bpf_log2l(lookahead_size));

    return 0;
}
"""

try:
    b = BPF(text=bpf_text)
except Exception as e:
    print(f"❌ 编译失败: {e}")
    print("⚠️  可能原因: 函数名不对、内核未导出该符号、或参数获取方式不兼容")
    sys.exit(1)

print("Tracing __do_page_cache_readahead() lookahead_size... Hit Ctrl-C to end.")

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
