#!/usr/bin/env python3
"""
python3-perf 使用示例
运行方式:
    sudo ./perf-python3-demo.py

参考源码:
    Linux 内核源码: tools/perf/python/
    - counting.py: 基本计数示例
    - twatch.py: 线程监控示例
    - tracepoint.py: tracepoint 监控示例
    - ilist.py: 交互式事件列表 (使用 textual)
"""

import sys
import os

# 检查是否使用系统 Python
if '/usr/bin/python3' not in sys.executable:
    print(f"警告: 当前使用的是 {sys.executable}")
    print("建议使用: /usr/bin/python3 perf-python3-demo.py")
    print()

try:
    import perf
except ImportError as e:
    print(f"错误: 无法导入 perf 模块: {e}")
    print("请确保已安装 python3-perf 包:")
    print("  Fedora/RHEL: sudo dnf install python3-perf")
    print("  Ubuntu/Debian: sudo apt install python3-perf")
    print()
    print("或者从内核源码编译:")
    print("  cd tools/perf && make")
    sys.exit(1)


def demo_counting():
    """示例1: 基本计数 - 类似于 perf stat"""
    print("=" * 60)
    print("示例1: 基本性能计数 (类似 perf stat)")
    print("=" * 60)

    # 使用 parse_events 解析事件字符串
    # 支持的事件格式与 perf 命令行相同
    events_str = "cpu-clock,task-clock,instructions,cycles"

    print(f"\n解析事件: {events_str}")

    try:
        evlist = perf.parse_events(events_str)
    except Exception as e:
        print(f"解析事件失败: {e}")
        return

    print(f"成功创建 {sum(1 for _ in evlist)} 个事件选择器")

    # 设置读取格式，获取运行时间和启用时间
    for evsel in evlist:
        evsel.read_format = perf.FORMAT_TOTAL_TIME_ENABLED | perf.FORMAT_TOTAL_TIME_RUNNING

    try:
        # 打开事件
        evlist.open()
        print("事件已打开")

        # 启用事件
        evlist.enable()
        print("事件已启用，开始执行工作负载...")

        # 执行一些工作负载
        count = 10000000
        while count > 0:
            count -= 1

        # 禁用事件
        evlist.disable()
        print("事件已禁用")

        # 读取并打印结果
        print("\n性能计数结果:")
        print("-" * 50)

        for evsel in evlist:
            # 遍历所有 CPU 和线程
            for cpu in evsel.cpus():
                for thread in evsel.threads():
                    counts = evsel.read(cpu, thread)
                    # counts 包含 val (值), ena (启用时间), run (运行时间)
                    print(f"  {evsel.name}:")
                    print(f"    CPU {cpu}, Thread {thread}:")
                    print(f"      Value: {counts.val:,}")
                    print(f"      Enabled: {counts.ena:,} ns")
                    print(f"      Running: {counts.run:,} ns")
                    if counts.ena > 0:
                        # 计算事件速率
                        scale = counts.run / counts.ena if counts.ena > 0 else 1.0
                        print(f"      Scaled Value: {counts.val * scale:,.0f}")

        evlist.close()
        print("事件列表已关闭")

    except PermissionError:
        print("错误: 权限不足，无法打开硬件性能计数器")
        print("请尝试:")
        print("  1. 使用 sudo 运行")
        print("  2. 调整 /proc/sys/kernel/perf_event_paranoid 为 -1 或 0")
    except Exception as e:
        print(f"错误: {e}")
        import traceback
        traceback.print_exc()

    print()


def demo_software_events():
    """示例2: 软件事件 - 不需要特殊硬件支持"""
    print("=" * 60)
    print("示例2: 软件事件 (页错误、上下文切换等)")
    print("=" * 60)

    # 软件事件通常不需要 root 权限
    events_str = "task-clock,context-switches,cpu-migrations,page-faults"

    print(f"\n解析事件: {events_str}")

    try:
        evlist = perf.parse_events(events_str)
    except Exception as e:
        print(f"解析事件失败: {e}")
        return

    try:
        evlist.open()
        evlist.enable()

        print("执行内存分配和进程操作...")

        # 分配一些内存，可能触发页错误
        data = []
        for i in range(100):
            data.append(bytearray(1024 * 1024))  # 1MB

        evlist.disable()

        print(f"\n分配了 {len(data)} MB 内存")
        print("\n软件事件计数:")
        print("-" * 50)

        for evsel in evlist:
            for cpu in evsel.cpus():
                for thread in evsel.threads():
                    counts = evsel.read(cpu, thread)
                    print(f"  {evsel.name}: {counts.val:,}")

        evlist.close()

    except Exception as e:
        print(f"错误: {e}")

    print()


def demo_thread_monitoring():
    """示例3: 线程生命周期监控 (类似于 twatch.py)"""
    print("=" * 60)
    print("示例3: 线程生命周期监控")
    print("=" * 60)
    print("""
此示例展示了如何监控进程的创建、退出和名称变更事件。
代码参考: tools/perf/python/twatch.py

注意: 需要 root 权限才能监控其他进程
""")

    example_code = '''
import perf

# 创建 CPU 和线程映射
cpus = perf.cpu_map()
threads = perf.thread_map()  # -1 表示所有线程

# 创建 DUMMY 软件事件
# 用于接收进程生命周期事件 (fork, exit, comm)
evsel = perf.evsel(
    type=perf.TYPE_SOFTWARE,
    config=perf.COUNT_SW_DUMMY,
    task=True,          # 启用任务事件跟踪
    comm=True,          # 启用进程名称变更跟踪
    mmap=False,
    freq=0,
    wakeup_events=1,
    sample_type=perf.SAMPLE_PERIOD | perf.SAMPLE_TID | perf.SAMPLE_CPU
)

# 打开事件
evsel.open(cpus=cpus, threads=threads)

# 创建事件列表
evlist = perf.evlist(cpus, threads)
evlist.add(evsel)
evlist.mmap()

# 事件循环
while True:
    # 等待事件，-1 表示无限等待
    evlist.poll(timeout=-1)

    # 遍历所有 CPU 读取事件
    for cpu in cpus:
        event = evlist.read_on_cpu(cpu)
        if not event:
            continue

        # 处理不同类型的事件
        if event.type == perf.RECORD_COMM:
            print(f"进程重命名: PID={event.sample_pid} "
                  f"Comm={event.sample_comm}")
        elif event.type == perf.RECORD_EXIT:
            print(f"进程退出: PID={event.sample_pid}")
        elif event.type == perf.RECORD_FORK:
            print(f"进程创建: PID={event.sample_pid} "
                  f"PPID={event.sample_ppid}")
        elif event.type == perf.RECORD_SWITCH:
            print(f"上下文切换: CPU={event.sample_cpu} "
                  f"PID={event.sample_pid}")
'''
    print(example_code)


def demo_tracepoint():
    """示例4: Tracepoint 监控 (类似于 tracepoint.py)"""
    print("=" * 60)
    print("示例4: Tracepoint 事件监控")
    print("=" * 60)
    print("""
此示例展示了如何监控内核 tracepoint 事件。
代码参考: tools/perf/python/tracepoint.py

常用 tracepoint:
  - sched:sched_switch     - 上下文切换
  - sched:sched_wakeup     - 任务唤醒
  - syscalls:sys_enter_*   - 系统调用入口
  - syscalls:sys_exit_*    - 系统调用退出
  - block:block_rq_issue   - 块设备请求
""")

    example_code = '''
import perf

# 创建 CPU 和线程映射
cpus = perf.cpu_map()
threads = perf.thread_map(-1)  # -1 表示所有线程

# 使用 parse_events 解析 tracepoint 事件
evlist = perf.parse_events("sched:sched_switch", cpus, threads)

# 禁用不必要的跟踪功能
for ev in evlist:
    ev.tracking = False

# 配置事件 (应用默认记录选项)
evlist.config()

# 简化 sample_type 和 read_format
for ev in evlist:
    ev.sample_type = ev.sample_type & ~perf.SAMPLE_IP
    ev.read_format = 0

# 打开、映射并启用事件
evlist.open()
evlist.mmap()
evlist.enable()

# 事件循环
while True:
    evlist.poll(timeout=-1)

    for cpu in cpus:
        event = evlist.read_on_cpu(cpu)
        if not event:
            continue

        # 只处理采样事件
        if not isinstance(event, perf.sample_event):
            continue

        # 打印上下文切换信息
        print("time %u prev_comm=%s prev_pid=%d prev_prio=%d "
              "prev_state=0x%x ==> next_comm=%s next_pid=%d next_prio=%d" % (
            event.sample_time,
            event.prev_comm,
            event.prev_pid,
            event.prev_prio,
            event.prev_state,
            event.next_comm,
            event.next_pid,
            event.next_prio
        ))
'''
    print(example_code)

    # 显示可用的 tracepoint
    print("\n可用的 tracepoint 子系统:")
    tracepoint_path = "/sys/kernel/debug/tracing/events"
    if os.path.exists(tracepoint_path):
        try:
            subsystems = [d for d in os.listdir(tracepoint_path)
                         if os.path.isdir(os.path.join(tracepoint_path, d))
                         and not d.startswith('.')][:10]
            for subsys in subsystems:
                print(f"  - {subsys}")
            print("  ... (更多请查看 /sys/kernel/debug/tracing/events/)")
        except PermissionError:
            print("  (需要 root 权限查看)")
    else:
        print(f"  {tracepoint_path} 不存在")
        print("  请确保已挂载 debugfs: mount -t debugfs none /sys/kernel/debug")
    print()


def demo_advanced_parsing():
    """示例5: 高级事件解析"""
    print("=" * 60)
    print("示例5: 高级事件解析")
    print("=" * 60)

    print("""
perf.parse_events() 支持多种事件格式:

1. 硬件事件:
   - cycles, instructions, cache-references, cache-misses
   - branches, branch-misses, bus-cycles

2. 软件事件:
   - cpu-clock, task-clock
   - context-switches, cpu-migrations
   - page-faults, minor-faults, major-faults

3. 缓存事件:
   - L1-dcache-loads, L1-dcache-load-misses
   - LLC-loads, LLC-load-misses
   - dTLB-loads, dTLB-load-misses

4. Tracepoint:
   - sched:sched_switch
   - syscalls:sys_enter_openat
   - block:block_rq_issue

5. 原始硬件事件:
   - rNNN (硬件原始编码)

6. 带修饰符的事件:
   - cycles:k (仅内核态)
   - cycles:u (仅用户态)
   - cycles:u,k (用户态和内核态)
   - cycles:p (精确事件)
""")

    # 测试一些事件解析
    test_events = [
        "cycles,instructions",
        "cache-references,cache-misses",
        "branch-instructions,branch-misses",
        "task-clock,context-switches",
    ]

    for events_str in test_events:
        try:
            evlist = perf.parse_events(events_str)
            count = sum(1 for _ in evlist)
            print(f"✓ '{events_str}' -> {count} 个事件")
        except Exception as e:
            print(f"✗ '{events_str}' -> 错误: {e}")

    print()


def demo_metrics():
    """示例6: 性能指标 (Metrics)"""
    print("=" * 60)
    print("示例6: 性能指标 (Metrics)")
    print("=" * 60)

    print("\n可用的性能指标:")
    try:
        metrics = list(perf.metrics())
        # 只显示前 10 个
        for metric in metrics[:10]:
            name = metric.get("MetricName", "Unknown")
            pmu = metric.get("PMU", "")
            desc = metric.get("BriefDescription", "")
            if pmu:
                print(f"  - {name} ({pmu}): {desc}")
            else:
                print(f"  - {name}: {desc}")

        if len(metrics) > 10:
            print(f"  ... (还有 {len(metrics) - 10} 个指标)")
    except Exception as e:
        print(f"无法获取指标列表: {e}")

    print("\n使用指标示例:")
    example_code = '''
# 解析并计算指标
evlist = perf.parse_metrics("IPC")
evlist.open()
evlist.enable()

# 执行工作负载...

evlist.disable()

# 计算指标值
for cpu in evlist.all_cpus():
    for thread in evlist.all_threads():
        value = evlist.compute_metric("IPC", cpu, thread)
        print(f"CPU {cpu}, Thread {thread}: IPC = {value}")
'''
    print(example_code)


def demo_pmus():
    """示例7: PMU (Performance Monitoring Unit) 列表"""
    print("=" * 60)
    print("示例7: 可用的 PMU")
    print("=" * 60)

    print("\nPMU (性能监控单元) 列表:")
    try:
        pmus = list(perf.pmus())
        for pmu in pmus[:10]:
            name = pmu.name()
            print(f"  - {name}")

            # 显示该 PMU 的前几个事件
            try:
                events = list(pmu.events())
                for event in events[:3]:
                    if "name" in event:
                        print(f"      {event['name']}: {event.get('desc', '')[:50]}...")
            except Exception as e:
                print(f"      (无法读取事件: {e})")

        if len(pmus) > 10:
            print(f"  ... (还有 {len(pmus) - 10} 个 PMU)")
    except Exception as e:
        print(f"无法获取 PMU 列表: {e}")

    print()


def check_permissions():
    """检查 perf 权限"""
    print("=" * 60)
    print("权限检查")
    print("=" * 60)

    # 检查 perf_event_paranoid
    try:
        with open("/proc/sys/kernel/perf_event_paranoid", "r") as f:
            paranoid = int(f.read().strip())
        print(f"perf_event_paranoid = {paranoid}")
        if paranoid == -1:
            print("  → 不限制 (允许访问所有事件)")
        elif paranoid == 0:
            print("  → 允许访问用户态事件")
        elif paranoid == 1:
            print("  → 允许访问用户态和内核态事件 (默认)")
        elif paranoid == 2:
            print("  → 仅允许访问用户态事件")
        else:
            print("  → 限制最严格")
    except Exception as e:
        print(f"无法读取 perf_event_paranoid: {e}")

    # 检查 debugfs
    print()
    if os.path.exists("/sys/kernel/debug/tracing"):
        print("debugfs: 已挂载 ✓")
    else:
        print("debugfs: 未挂载 ✗")
        print("  运行: mount -t debugfs none /sys/kernel/debug")

    # 检查当前用户
    print()
    if os.geteuid() == 0:
        print("当前用户: root (可以访问所有功能) ✓")
    else:
        print(f"当前用户 UID: {os.geteuid()}")
        print("注意: 某些功能可能需要 root 权限")

    print()


def main():
    print("\n" + "=" * 60)
    print("python3-perf 使用示例")
    print("=" * 60)
    print()
    print("python3-perf 是 Linux perf 工具的 Python 绑定")
    print("提供了对 perf_event_open 系统调用的 Python 接口")
    print()
    print("参考源码: Linux 内核 tools/perf/python/")
    print()

    check_permissions()

    demos = [
        ("基本性能计数", demo_counting),
        ("软件事件", demo_software_events),
        ("线程监控 (代码示例)", demo_thread_monitoring),
        ("Tracepoint 监控 (代码示例)", demo_tracepoint),
        ("高级事件解析", demo_advanced_parsing),
        ("性能指标", demo_metrics),
        ("PMU 列表", demo_pmus),
    ]

    # 如果没有参数，运行所有 demo
    if len(sys.argv) > 1:
        try:
            demo_num = int(sys.argv[1])
            if 1 <= demo_num <= len(demos):
                demos[demo_num - 1][1]()
            else:
                print(f"无效的 demo 编号: {demo_num}")
                print(f"可用范围: 1-{len(demos)}")
        except ValueError:
            print(f"用法: {sys.argv[0]} [demo_number]")
            print("\n可用的 demo:")
            for i, (name, _) in enumerate(demos, 1):
                print(f"  {i}. {name}")
    else:
        # 运行所有 demo
        for name, func in demos:
            try:
                func()
            except KeyboardInterrupt:
                print(f"\n{name} 被用户中断")
            except Exception as e:
                print(f"\n{name} 出错: {e}")

    print("=" * 60)
    print("示例结束")
    print("=" * 60)
    print()
    print("参考资源:")
    print("  - Linux 内核源码: tools/perf/python/")
    print("  - tuned 项目: https://github.com/redhat-performance/tuned")
    print("  - perf 文档: https://man7.org/linux/man-pages/man2/perf_event_open.2.html")
    print()
    print("运行特定 demo: python3 perf-python3-demo.py <编号>")


if __name__ == '__main__':
    main()
