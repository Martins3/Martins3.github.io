#!/usr/bin/python3
from bcc import BPF
from time import sleep, strftime, localtime
import signal
import sys

# 用于存储每秒统计结果的缓冲区
output_buffer = []

# BPF 程序
bpf_text = """
#include <uapi/linux/ptrace.h>

BPF_PERCPU_ARRAY(exec_count, u64, 1);

int trace_execve(struct pt_regs *ctx) {
    u64 zero = 0, *val;
    int key = 0;

    val = exec_count.lookup_or_try_init(&key, &zero);
    if (val) {
        (*val)++;
    }

    return 0;
}
"""

# 加载 BPF 程序
b = BPF(text=bpf_text)
b.attach_tracepoint(tp="syscalls:sys_enter_execve", fn_name="trace_execve")

print("Tracing execve() calls... Hit Ctrl-C to end.")

key = 0

# Ctrl-C 信号处理函数：将缓冲区写入文件并退出
def signal_handler(sig, frame):
    print("\nReceived Ctrl-C, writing data to /tmp/exec.txt...")
    try:
        with open("/tmp/exec.txt", "w") as f:
            f.write("Timestamp,ExecveCalls\n")
            for line in output_buffer:
                f.write(line + "\n")
        print("Data saved to /tmp/exec.txt. Exiting.")
    except Exception as e:
        print(f"Error writing file: {e}")
    sys.exit(0)

# 注册信号处理器
signal.signal(signal.SIGINT, signal_handler)

try:
    while True:
        sleep(1)

        # 获取当前时间戳
        timestamp = strftime("%Y-%m-%d %H:%M:%S", localtime())

        # 获取 execve 调用总数
        total = b["exec_count"].sum(key).value

        # 重置计数器
        b["exec_count"].__setitem__(key, b["exec_count"].Leaf(0))

        # 格式化输出并存入缓冲区
        line = f"{timestamp},{total}"
        output_buffer.append(line)

        # 同时也打印到终端（可选，方便实时观察）
        print(line)

except KeyboardInterrupt:
    # 实际上会被 signal_handler 捕获，这里保留以防万一
    pass
