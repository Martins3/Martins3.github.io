# cstate

## 基本流程

`do_idle()` 是统一入口：先判断是否轮询，否则进入 `cpuidle_idle_call()`；
后者在 cpuidle 可用时选择并进入 idle state，不可用时回退到架构默认 idle。
下面按当前 linux-drm 源码整理，省略调度检查、统计和部分特殊平台分支。

```text
do_idle()
├─ 强制轮询，或广播定时器事件已到期
│  ├─ tick_nohz_idle_restart_tick()
│  └─ cpu_idle_poll()                       # 忙等；x86 可用 idle=poll 强制选择
└─ cpuidle_idle_call()
   ├─ cpuidle_get_device()                  # 当前 CPU 的 cpuidle device
   ├─ cpuidle_get_cpu_driver()              # 对应的 cpuidle driver
   │
   ├─ cpuidle_not_available()：             # **cpuidle 不可用**
   │  ├─ idle_call_stop_or_retain_tick()    # 按条件停止或保留调度 tick
   │  └─ default_idle_call()
   │     └─ arch_cpu_idle()
   │        └─ static_call(x86_idle)()      # 以下为 x86 常见实现
   │           ├─ default_idle()
   │           │  └─ raw_safe_halt()
   │           │     └─ arch_safe_halt()
   │           │        └─ native_safe_halt()  # 原生实现：STI; HLT
   │           └─ mwait_idle()                 # MONITOR; STI; MWAIT，默认 C1
   │
   └─ cpuidle 可用
      ├─ idle_should_enter_s2idle()，或设置了 forced_idle_latency_limit_ns
      │  ├─ s2idle 时先尝试 call_cpuidle_s2idle()
      │  │  └─ cpuidle_enter_s2idle()
      │  │     └─ enter_s2idle_proper() → cpuidle_state::enter_s2idle()
      │  │        # 选择满足延迟限制的最深 s2idle 状态；成功后返回
      │  └─ s2idle 路径未进入状态，或为强制 idle
      │     ├─ tick_nohz_idle_stop_tick()
      │     ├─ cpuidle_find_deepest_state() # 绕过 governor，按延迟限制选最深状态
      │     └─ call_cpuidle()
      ├─ 普通 idle，且 driver 提供多个状态
      │  ├─ cpuidle_select()
      │  │  └─ cpuidle_governor::select()  # 选择状态，并给出是否停止 tick 的建议
      │  ├─ idle_call_stop_or_retain_tick()
      │  │  ├─ tick_nohz_idle_stop_tick()  # 建议停止，或 tick 已经停止
      │  │  └─ tick_nohz_idle_retain_tick() # 否则保留 tick
      │  ├─ call_cpuidle()
      │  └─ cpuidle_reflect()              # 唤醒后将实际进入的状态反馈给 governor
      └─ 普通 idle，只有一个状态
         ├─ idle_call_stop_or_retain_tick()
         └─ call_cpuidle(..., 0)           # 无需 governor 选择，直接使用 state 0

上面各处 call_cpuidle() 的常见进入路径：
call_cpuidle()
└─ cpuidle_enter()
   └─ cpuidle_enter_state()
      ├─ tick_broadcast_enter()            # 仅在状态带 CPUIDLE_FLAG_TIMER_STOP 时
      └─ cpuidle_state::enter()            # driver 回调实际执行 HLT、MWAIT 等操作
```

在 guest 中，若 hypervisor 启用了 HLT 拦截，`default_idle()` 最终执行的 HLT 会导致 VM exit；
如果关闭了 HLT 拦截，则不会仅因该指令而退出。原生实现如下：

```c
static __always_inline void native_safe_halt(void)
{
	x86_idle_clear_cpu_buffers();
	asm volatile("sti; hlt": : :"memory");
}
```

## 使用指令是什么?

### pause

PAUSE 更像是给原有自旋循环加了一条“我正在等”的提示。它不负责检测条件，也不需要其他 CPU 专门唤醒；执行完后，程序自己再次检查条件。Intel
系统编程手册 (https://www.intel.com/content/dam/support/us/en/documents/processors/pentium4/sb/25366821.pdf)

还有一个很有意思的兼容设计：PAUSE 的编码是 F3 90，也就是 REP NOP。老处理器可以把它作为 NOP
执行，新处理器则识别出自旋提示。因此它能在保持程序逻辑不变的前提下优化等待。你看到某些旧代码把 cpu_relax() 写成 rep;
nop，表达的也是这个意思。Intel 指令手册：PAUSE
(https://www.intel.com/content/dam/www/public/us/en/documents/manuals/64-ia-32-architectures-software-developer-vol-2b-manual.pdf)

cpu_relax() 是内核用于忙等循环的架构相关提示指令。在你当前这份源码中，x86 最终执行 PAUSE，ARM64 执行 YIELD。

x86 实现在 arch/x86/include/asm/vdso/processor.h:11 (arch/x86/include/asm/vdso/processor.h:11)：

```cpp
static __always_inline void native_pause(void)
{
asm volatile("pause" ::: "memory");
}
```

```cpp
static __always_inline void cpu_relax(void)
{
native_pause();
}
```




### hlt && monitor/mwait
• HLT 在 8086/8088 时代就有，提供最基本的“停止执行，等待中断”的能力。Intel ASM86 手册
  (https://community.intel.com/cipcp26785/attachments/cipcp26785/c-compiler/31103/1/121703-003_ASM86_Language_Reference_Manual_Nov83.pdf)

• MONITOR/MWAIT 到 Pentium 4 的 SSE3 时代才引入，针对多线程同步、共享执行资源等需求增加了新的等待方式，所以也已经是二十多年前的技术。Intel
  架构手册 (https://cdrdv2-public.intel.com/774491/253669-sdm-vol-3b.pdf)

它解决的一个具体问题是：

HLT：
CPU 1 写入“有工作” → 发送 IPI → CPU 0 被唤醒

MONITOR + MWAIT：
CPU 0 已监视通知地址
CPU 1 写入“有工作” → 硬件监视触发 → CPU 0 退出等待

这样可以避免某些场景下发送、处理中断的开销。Intel 手册明确使用 complement（补充） 来描述 MONITOR/MWAIT 与 HLT/PAUSE 的关系。Intel
多处理器管理说明 (https://cdrdv2-public.intel.com/671294/252046-sdm-change-document.pdf)

在 Linux 的 CPU idle 场景里，它们又确实存在“优先用较新机制，不适用时回退”的关系。 你当前源码中的 prefer_mwait_c1_over_halt() (arch/x86/
kernel/process.c:868) 就明确表达了：

• 支持且满足条件时，优先用 MWAIT。
• 不支持、有相关硬件问题，或指定 idle=nomwait 时，回退到 HLT。

因此，你把 MWAIT 理解为为多线程同步和电源管理增加能力的后续机制很合适；而 HLT 仍然保留着简单、基础、兼容性好的等待接口这一角色。


一般来说，内核通过 MONITOR + MWAIT 指令请求进入 C6，
每次进入时不需要调用 BIOS/UEFI 或 ACPI 方法。
调用路径是：

```txt
cpuidle governor 选择 C6
  → intel_idle()
  → mwait_idle_with_hints(0x20, 1)
  → MONITOR
  → MWAIT
```

对应 drivers/idle/intel_idle.c 的 intel_idle() 和 arch/x86/include/asm/mwait.h 的 mwait_idle_with_hints()。

两条指令各有作用：
- MONITOR：监视当前线程的 flags 所在内存区域，让其他 CPU 设置重新调度标志时可以唤醒它。内核会在设置监视后再次检查是否需要调度，避免遗漏唤醒。
- MWAIT：实际进入等待。这里 EAX=0x20 是该型号的 C6 请求，ECX=1 表示允许中断打断等待，即使当时屏蔽了可屏蔽中断。

0x20 是 hint，请求而非保证。最终能进入多深的状态，由 CPU 内部电源管理逻辑结合硬件配置、同一核心另一个超线程是否空闲等条件决定。内核文档
(https://www.kernel.org/doc/html/latest/admin-guide/pm/intel_idle.html#enumeration-of-idle-states)

固件的作用需要分阶段看：

 阶段               固件的作用
━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 开机初始化         配置电源管理寄存器、允许的状态及限制，并通过 ACPI 提供状态信息
─────────────────  ────────────────────────────────────────────────────────────────
 每次进入核心 C6    OS 执行 MWAIT，CPU 内部完成状态转换，无需经过 BIOS 调用
─────────────────  ────────────────────────────────────────────────────────────────
 实际能否进入       仍受固件预先设置的硬件限制影响

## cstate 一共有多少层

不同的机器不同的的

1. 从 drivers/idle/intel_idle.c::intel_idle_ids 就可以看出来，
这说明具体有多少层都是代码中定义枚举出来的。
2. intel_idle_init_cstates_acpi_lpi 可以看到 cstate 是从 acpi 中获取的

cstate 可以走 acpi ，也可以走 intel_idle ，所以在 intel_idle 和 acpi 都有定义，比较合理了。


13900k 中分析:
```txt
🧀  cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 21:

Number of idle states: 4
Available idle states: POLL C1_ACPI C2_ACPI C3_ACPI
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 49122410
Duration: 195515238
C1_ACPI:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 591051105
Duration: 105530636171
C2_ACPI:
Flags/Description: ACPI FFH MWAIT 0x21
Latency: 127
Usage: 205388762
Duration: 147772318872
C3_ACPI:
Flags/Description: ACPI FFH MWAIT 0x60
Latency: 1048
Usage: 5242208
Duration: 427034713752
```

而且 n100 的机器上有 C10 状态，而且一共存在 5 个状态
```txt
~ 🐶 took 6s
🧀  cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 2:

Number of idle states: 5
Available idle states: POLL C1E C6 C8 C10
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 24349
Duration: 957424
C1E:
Flags/Description: MWAIT 0x01
Latency: 2
Usage: 5298721
Duration: 493190555
C6:
Flags/Description: MWAIT 0x20
Latency: 195
Usage: 1699893
Duration: 1490308215
C8:
Flags/Description: MWAIT 0x40
Latency: 260
Usage: 65316
Duration: 514509727
C10:
Flags/Description: MWAIT 0x60
Latency: 660
Usage: 11350519
Duration: 857692959524
```


## cstate 会导致性能下降，但是延迟降低

sudo cgexec --sticky -g cpu,memory,cpuset:/ taskset -ac 30 /usr/bin/sysbench cpu --cpu-max-prime=1000000 run --threads=1


### INTEL(R) XEON(R) GOLD 6530

#### 第一轮测试

```sh
# events per second:     5.81
# events per second:     3.96
# events per second:     3.96
# events per second:     5.81

configs=(
	["0"]=""
	["1"]="intel_idle.max_cstate=0 processor.max_cstate=1 intel_pstate=disable"
	["2"]="intel_idle.max_cstate=0 processor.max_cstate=1"
	["3"]="intel_idle.max_cstate=0"
	["4"]="intel_pstate=disable"
	["5"]=""
)
```

可以看到，当禁用了 cstate 性能下降，但是 intel_pstate 被禁用就没什么影响。

#### 第二轮测试
上测试，测试方法分别为:

sudo cgexec --sticky -g cpu,memory,cpuset:/ taskset -ac 30 /usr/bin/sysbench cpu --cpu-max-prime=1000000 run --threads=1

/usr/bin/sysbench cpu --cpu-max-prime=1000000 run --threads=8

所以，似乎性能的关键在于 cstate 是否打开啊。

首先关掉所有的 cstate ，测试频率为
```txt
     24 cpu MHz         : 2699
    104 cpu MHz         : 2700
```

```txt
    events per second:     3.88
```

```txt
    events per second:    46.39
```

然后打开 C6，也就是
sudo cpupower idle-set -e 3

```txt
🧀  sudo cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 0:

Number of idle states: 4
Available idle states: POLL C1 C1E C6
POLL (DISABLED) :
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 335805
Duration: 21370333
C1 (DISABLED) :
Flags/Description: MWAIT 0x00
Latency: 1
Usage: 256757
Duration: 64096263
C1E (DISABLED) :
Flags/Description: MWAIT 0x01
Latency: 2
Usage: 18532835
Duration: 13457686591
C6:
Flags/Description: MWAIT 0x20
Latency: 290
Usage: 25416114
Duration: 38744715490
```

```txt

      1 cpu MHz         : 3751
      1 cpu MHz         : 3800
      1 cpu MHz         : 3830
      1 cpu MHz         : 3869
      1 cpu MHz         : 3901
      1 cpu MHz         : 3934
      1 cpu MHz         : 3947
      1 cpu MHz         : 3952
      1 cpu MHz         : 3968
      1 cpu MHz         : 3974
      1 cpu MHz         : 3977
      2 cpu MHz         : 3987
      1 cpu MHz         : 3991
      2 cpu MHz         : 3992
      1 cpu MHz         : 3995
      1 cpu MHz         : 3996
      1 cpu MHz         : 3997
      1 cpu MHz         : 3998
      3 cpu MHz         : 3999
    105 cpu MHz         : 4000
```

测试结果为:
```txt
    events per second:     5.80
```

```txt
    events per second:    30.97
```

### 13th Gen Intel(R) Core(TM) i9-13900K

关闭所有的 cstate ，结果反而是 CPU 在满频率跑
```txt
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5499.944
cpu MHz         : 5500.055
cpu MHz         : 5500.055
cpu MHz         : 5500.000
cpu MHz         : 5499.944
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.000
cpu MHz         : 5500.055
cpu MHz         : 4300.012
cpu MHz         : 4300.034
cpu MHz         : 4300.000
cpu MHz         : 4300.004
cpu MHz         : 4300.006
cpu MHz         : 4300.000
cpu MHz         : 4299.889
cpu MHz         : 4300.023
cpu MHz         : 4300.028
cpu MHz         : 4300.076
cpu MHz         : 4299.930
cpu MHz         : 4299.951
cpu MHz         : 4300.084
cpu MHz         : 4299.981
cpu MHz         : 4299.920
cpu MHz         : 4299.945
```

```txt
    events per second:     7.97
    events per second:    63.55
```

如果打开 c6，结果为:
sudo cpupower idle-set -e 3
```txt
   events per second:     8.27
   events per second:    63.77
```

如果打开所有
```txt
   events per second:     8.08
   events per second:     8.12
   events per second:    63.72
```
只能说，对于 13900k ，Cstate 的打开与否，差别不大。

### Intel(R) Xeon(R) Gold 5218 CPU @ 2.30GHz
sudo cpupower idle-set -e 3

sysbench cpu --cpu-max-prime=1000000 run --threads=1

events per second:     1.60 (关闭 c6)
events per second:     2.20 (打开 c6)


### Hygon C86 7380 32-core Processor
(全部都打开)
  events per second:     2.13
  events per second:     2.11

  events per second:     2.06
  events per second:     2.04

(仅仅关闭 c3)
   events per second:     1.87
   events per second:     1.85

### Intel(R) Xeon(R) Gold 6442Y

```txt
$ sudo cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 0:

Number of idle states: 4
Available idle states: POLL C1 C1E C6
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 1182456
Duration: 3423380
C1:
Flags/Description: MWAIT 0x00
Latency: 1
Usage: 4159557
Duration: 412899749
C1E:
Flags/Description: MWAIT 0x01
Latency: 2
Usage: 3816990355
Duration: 3649371734916
C6:
Flags/Description: MWAIT 0x20
Latency: 290
Usage: 1746598455
Duration: 1831095620948
```

96 thread 的机器，一共存在 8 个 core 被 busy polling 轮询

关闭 c6 的性能:
	events per second:     4.54

打开 c6 的性能
	events per second:     5.40


## cstate 对于网络性能影响

### Intel(R) Xeon(R) Gold 6442Y

sudo nerdctl run --privileged -it --rm --network host networkstatic/iperf3 -c 10.0.129.145 -t 0

关闭之后，为 16.0 Gbits/sec 和 16.5 Gbits/sec

### 13900K

#### ping 延迟下降 10 倍
可以的，而且非常明显，

配置两个虚拟机为 vhost-net 网络，从一个虚拟机中 ping 另外一个虚拟机 fping -l 10.0.14.0

cstate 后，每一级都关闭
```txt
10.0.14.0 : [198], 64 bytes, 0.093 ms (0.348 avg, 0% loss)
10.0.14.0 : [199], 64 bytes, 0.103 ms (0.347 avg, 0% loss)
10.0.14.0 : [200], 64 bytes, 0.074 ms (0.345 avg, 0% loss)
10.0.14.0 : [201], 64 bytes, 0.063 ms (0.344 avg, 0% loss)
10.0.14.0 : [202], 64 bytes, 0.081 ms (0.343 avg, 0% loss)
10.0.14.0 : [203], 64 bytes, 0.078 ms (0.341 avg, 0% loss)
10.0.14.0 : [204], 64 bytes, 0.102 ms (0.340 avg, 0% loss)
10.0.14.0 : [205], 64 bytes, 0.065 ms (0.339 avg, 0% loss)
10.0.14.0 : [206], 64 bytes, 0.114 ms (0.338 avg, 0% loss)
10.0.14.0 : [207], 64 bytes, 0.078 ms (0.336 avg, 0% loss)
```

如果全部打开后，结果为:
```txt
10.0.14.0 : [42], 64 bytes, 1.09 ms (0.321 avg, 0% loss)
10.0.14.0 : [43], 64 bytes, 0.779 ms (0.331 avg, 0% loss)
10.0.14.0 : [44], 64 bytes, 0.913 ms (0.344 avg, 0% loss)
10.0.14.0 : [45], 64 bytes, 1.18 ms (0.362 avg, 0% loss)
10.0.14.0 : [46], 64 bytes, 0.773 ms (0.371 avg, 0% loss)
10.0.14.0 : [47], 64 bytes, 0.789 ms (0.380 avg, 0% loss)
```

#### iperf3 性能提升

```txt
[  5]  48.00-49.00  sec  4.53 GBytes  38.9 Gbits/sec
[  5]  49.00-50.00  sec  4.58 GBytes  39.4 Gbits/sec
[  5]  50.00-51.00  sec  4.56 GBytes  39.2 Gbits/sec
[  5]  51.00-52.00  sec  4.57 GBytes  39.3 Gbits/sec
[  5]  52.00-53.00  sec  4.54 GBytes  39.0 Gbits/sec
[  5]  53.00-54.00  sec  4.29 GBytes  36.9 Gbits/sec (如果 sudo cpupower -d 1)
[  5]  54.00-55.00  sec  3.66 GBytes  31.4 Gbits/sec
[  5]  55.00-56.00  sec  3.70 GBytes  31.8 Gbits/sec
[  5]  56.00-57.00  sec  3.68 GBytes  31.6 Gbits/sec
[  5]  57.00-58.00  sec  3.70 GBytes  31.8 Gbits/sec
[  5]  58.00-59.00  sec  3.64 GBytes  31.3 Gbits/sec
[  5]  59.00-60.00  sec  3.66 GBytes  31.5 Gbits/sec
[  5]  60.00-61.00  sec  3.70 GBytes  31.8 Gbits/sec
[  5]  61.00-62.00  sec  3.66 GBytes  31.4 Gbits/sec
[  5]  62.00-63.00  sec  3.76 GBytes  32.3 Gbits/sec
[  5]  63.00-64.00  sec  3.66 GBytes  31.4 Gbits/sec
[  5]  64.00-65.00  sec  3.73 GBytes  32.0 Gbits/sec
[  5]  65.00-66.00  sec  3.73 GBytes  32.1 Gbits/sec
[  5]  66.00-67.00  sec  3.71 GBytes  31.8 Gbits/sec
```

## 如果所有的核心跑满，cstate 的修改意义不大
<!-- 62ce5cbb-4380-4f69-896a-308ce73003b2 -->

### Intel(R) Xeon(R) Gold 5218 CPU @ 2.30GHz
打开所有的 cstate
```txt
🧀  /usr/bin/sysbench cpu --cpu-max-prime=1000000 run --threads=$(nproc --all)

CPU speed:
    events per second:   253.46
```

关闭所有的 cstate
```txt
🧀  /usr/bin/sysbench cpu --cpu-max-prime=1000000 run --threads=$(nproc --all)
sysbench 1.0.17-41cf132 (using bundled LuaJIT 2.1.0-beta2)

CPU speed:
    events per second:   251.31
```

基本没区别了。

## 参数的含义

acpi 和 intel_idle 分别都提供了自己的 idle driver:
```txt
/sys/module/processor/parameters/max_cstate
/sys/module/intel_idle/parameters/max_cstate
```
drivers/acpi/processor_idle.c

这是一个静态变量，所以 processor.max_cstate 是不可以影响到 intel 的 max_cstate 的
这两个参数是独立的。


### processor.max_cstate=0 的含义是什么?
<!-- bfa82985-464a-4bcc-90da-a89de7ceeee2 -->

drivers/acpi/processor_idle.c 中的确存在这样的逻辑
```sh
 if (max_cstate == 0)
	 max_cstate = 1;
```

设置之后，可以看到，最高就是 C1
```txt
$ sudo cpupower idle-info
CPUidle driver: acpi_idle
CPUidle governor: menu
analyzing CPU 0:

Number of idle states: 2
Available idle states: POLL C1
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 83
Duration: 2916
C1:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 366840
Duration: 355738830
```

### intel_idle.max_cstate=0 和 processor.max_cstate=1 是独立参数
<!-- a97bb930-3cbf-4a16-ac35-240c4fa97f76 -->

例如 processor.max_cstate=1 ，那么可以找到结果为:
```txt

$ sudo cpupower idle-info
CPUidle driver: intel_idle
CPUidle governor: menu
analyzing CPU 0:

Number of idle states: 3
Available idle states: POLL C1_ACPI C2_ACPI
POLL:
Flags/Description: CPUIDLE CORE POLL IDLE
Latency: 0
Usage: 25
Duration: 515
C1_ACPI:
Flags/Description: ACPI FFH MWAIT 0x0
Latency: 1
Usage: 2815
Duration: 168086
C2_ACPI:
Flags/Description: ACPI FFH MWAIT 0x20
Latency: 41
Usage: 301145
Duration: 310684949
```

## intel 服务器 cstate 的关闭会导致频率变化，同时相应的出现性能变化

1. 默认高频
2. 关闭 Cstate 导致进入到低频
3. 搞负载导致进入到低频

### INTEL(R) XEON(R) GOLD 6530
如果 sudo cpupower idle-set -e 2

那么 sudo cgexec -g cpu,memory,cpuset:/ sysbench cpu --cpu-max-prime=1000000 run --threads=1
结果的变化为:
```txt
    events per second:     3.95

    events per second:     5.81
```
其频率变化为:
```txt
      1 cpu MHz         : 2696
      1 cpu MHz         : 2698
      1 cpu MHz         : 2699
      2 cpu MHz         : 2700
```

```txt
     10 cpu MHz         : 4000
      2 cpu MHz         : 4001
      3 cpu MHz         : 4006
      2 cpu MHz         : 4007
```
这说明 aperfmperf 获取的频率和 sysbench 测试都是非常有说服力的。

但是需要指出的是，这种 cstate 导致频率的变化如此之大，不是普遍的，例如。
 13th Gen Intel(R) Core(TM) i9-13900K 关闭 C state 之后，所有的核心都会是
 4.3 和 5.5G Hz

### Intel(R) Xeon(R) Gold 6442Y

当关闭各级 cstate 的时候:
```txt
24 cpu MHz         : 3299
72 cpu MHz         : 3300
```
当打开全部
```txt
      2 cpu MHz         : 4001
      1 cpu MHz         : 4002
      2 cpu MHz         : 4003
      3 cpu MHz         : 4004
      5 cpu MHz         : 4005
      1 cpu MHz         : 4006
```
当高负载的时候:
```txt
     11 cpu MHz         : 3299
     85 cpu MHz         : 3300
```
### Intel(R) Xeon(R) Gold 5218 CPU @ 2.30GHz

```txt
    40 cpu MHz         : 3900
```
加压后
```txt
     35 cpu MHz         : 2799
     29 cpu MHz         : 2800
```
 sudo cpupower idle-set -e 1 效果类似

## 如何理解空载的时候，但是 CPU 频率很高
<!-- 0796869f-cbda-4432-bf3e-92065f9d938b -->

intel 服务器默认高频，加负载负载，频率变低

hygon 机器是反过来的，默认低频，加负载，变高

具体实现就不知道了，应该和硬件实现有关系吧。

## cstate
<!-- 5113fe79-d1bb-4cf4-acde-63d2662eaf69 -->

### cstate governor

定义在 drivers/cpuidle/governors/ 中四个文件中:
```c
static struct cpuidle_governor menu_governor = {
    .name =     "menu",
    .rating =   20,
    .enable =   menu_enable_device,
    .select =   menu_select,
    .reflect =  menu_reflect,
};
```

```c
static struct cpuidle_governor ladder_governor = {
    .name =     "ladder",
    .rating =   10,
    .enable =   ladder_enable_device,
    .select =   ladder_select_state,
    .reflect =  ladder_reflect,
};
```

```c
static struct cpuidle_governor haltpoll_governor = {
    .name =         "haltpoll",
    .rating =       9,
    .enable =       haltpoll_enable_device,
    .select =       haltpoll_select,
    .reflect =      haltpoll_reflect,
};

static struct cpuidle_governor teo_governor = {
	.name =		"teo",
	.rating =	19,
	.enable =	teo_enable_device,
	.select =	teo_select,
	.reflect =	teo_reflect,
};
```

### cstate driver

这就很多了:
```txt
🧀  rg -l "static struct cpuidle_driver"
arch/x86/kernel/apm_32.c
arch/sh/kernel/cpu/shmobile/cpuidle.c
arch/arm/mach-s3c/cpuidle-s3c64xx.c
arch/arm/mach-omap2/cpuidle44xx.c
arch/arm/mach-omap2/cpuidle34xx.c
arch/arm/mach-imx/cpuidle-imx7ulp.c
arch/arm/mach-imx/cpuidle-imx6sx.c
arch/arm/mach-imx/cpuidle-imx6sl.c
arch/arm/mach-imx/cpuidle-imx6q.c
arch/arm/mach-imx/cpuidle-imx5.c
drivers/cpuidle/sysfs.c
drivers/cpuidle/cpuidle-qcom-spm.c
drivers/cpuidle/driver.c
drivers/cpuidle/cpuidle-zynq.c
drivers/cpuidle/cpuidle-ux500.c
drivers/cpuidle/cpuidle-tegra.c
drivers/cpuidle/cpuidle-pseries.c
drivers/cpuidle/cpuidle-powernv.c
drivers/cpuidle/cpuidle-mvebu-v7.c
drivers/cpuidle/cpuidle-kirkwood.c
drivers/cpuidle/cpuidle-haltpoll.c
drivers/cpuidle/cpuidle-exynos.c
drivers/cpuidle/cpuidle-cps.c
drivers/cpuidle/cpuidle-clps711x.c
drivers/cpuidle/cpuidle-calxeda.c
drivers/cpuidle/cpuidle-big_little.c
drivers/cpuidle/cpuidle-at91.c
drivers/cpuidle/cpuidle-arm.c
drivers/idle/intel_idle.c
```


## 细节
### acpi cpuidle_driver 分析

drivers/acpi/acpi_processor.c

ACPI IOPORT 0x814 是从 acpi_processor_evaluate_cst

```plain
crash> struct cpuidle_driver 0xffffffffa2592960
struct cpuidle_driver {
  name = 0xffffffffa22ed8d9 "acpi_idle",
  owner = 0x0,
  refcnt = 0,
  bctimer = 0,
  states = { {
      name = "POLL\000\000\000\000\000\000\000\000\000\000\000",
      desc = "CPUIDLE CORE POLL IDLE\000\000\000\000\000\000\000\000\000",
      flags = 1,
      exit_latency = 0,
      power_usage = -1,
      target_residency = 0,
      disabled = false,
      enter = 0xffffffffa1acb610 <poll_idle>,
      enter_dead = 0x0,
      enter_s2idle = 0x0
    }, {
      name = "C1\000\000\000\000\000\000\000\000\000\000\000\000\000",
      desc = "ACPI HLT\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
      flags = 0,
      exit_latency = 1,
      power_usage = 0,
      target_residency = 2,
      disabled = false,
      enter = 0xffffffffa1731ad0 <acpi_idle_enter>,
      enter_dead = 0xffffffffa17318f0 <acpi_idle_play_dead>,
      enter_s2idle = 0x0
    }, {
      name = "C2\000\000\000\000\000\000\000\000\000\000\000\000\000",
      desc = "ACPI IOPORT 0x814\000\000\000\000\000\000\000\000\000\000\000\000\000\000",
      flags = 0,
      exit_latency = 400,
      power_usage = 0,
      target_residency = 800,
      disabled = false,
      enter = 0xffffffffa1731ad0 <acpi_idle_enter>,
      enter_dead = 0xffffffffa17318f0 <acpi_idle_play_dead>,
      enter_s2idle = 0xffffffffa1731a70 <acpi_idle_enter_s2idle>
    }, {
    /// .... 其他的都是 0
    }},
  state_count = 3,
  safe_state_index = 2,
  cpumask = 0xffffffffa2666c20 <__cpu_possible_mask>
}
```

和这个对应:
```c
static struct cpuidle_state snb_cstates[] __initdata = {
	{
		.name = "C1",
		.desc = "MWAIT 0x00",
		.flags = MWAIT2flg(0x00),
		.exit_latency = 2,
		.target_residency = 2,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C1E",
		.desc = "MWAIT 0x01",
		.flags = MWAIT2flg(0x01) | CPUIDLE_FLAG_ALWAYS_ENABLE,
		.exit_latency = 10,
		.target_residency = 20,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C3",
		.desc = "MWAIT 0x10",
		.flags = MWAIT2flg(0x10) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 80,
		.target_residency = 211,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C6",
		.desc = "MWAIT 0x20",
		.flags = MWAIT2flg(0x20) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 104,
		.target_residency = 345,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.name = "C7",
		.desc = "MWAIT 0x30",
		.flags = MWAIT2flg(0x30) | CPUIDLE_FLAG_TLB_FLUSHED,
		.exit_latency = 109,
		.target_residency = 345,
		.enter = &intel_idle,
		.enter_s2idle = intel_idle_s2idle, },
	{
		.enter = NULL }
};
```

### amd 机器上也存在 intel_idle

```txt
cd /sys/module/intel_idle/parameters🔒
🧀  l
Permissions Size User Date Modified Name
.r--r--r--  4.1k root 25 6月  10:38  force_irq_on
.r--r--r--  4.1k root 25 6月  10:38  max_cstate
.r--r--r--  4.1k root 25 6月  10:38  no_acpi
.r--r--r--  4.1k root 25 6月  10:38  preferred_cstates
.r--r--r--  4.1k root 25 6月  10:38  states_off
.r--r--r--  4.1k root 25 6月  10:38  use_acpi
```
其实这个是一个可以优化的内容，按道理的确不该加载的

## 代码细节
struct cpuidle_device 和 struct cpuidle_driver

在 cpuidle_idle_call 中:

```c
	struct cpuidle_device *dev = cpuidle_get_device();
	struct cpuidle_driver *drv = cpuidle_get_cpu_driver(dev);
```

通过 cpuidle_register_device 来注册到 percpu 中的 `cpuidle_devices`
```c
DEFINE_PER_CPU(struct cpuidle_device *, cpuidle_devices);
```

注册 driver 的方法: cpuidle_register_driver

1. intel_idle_init -> intel_idle_cpu_online -> intel_idle_cpu_init
2. acpi_processor_start -> __acpi_processor_start ->  acpi_processor_power_init

其实 device 似乎就是关联的具体的 cpu 而已，而 driver 的选择就是看到底是 acpi 还是 device 了:

## cstate

状态检查:
```txt
/sys/devices/system/cpu/cpu*/cpuidle/state*/name
/sys/devices/system/cpu/cpu*/cpuidle/state*/disable
```
禁用方法:
1. 内核参数
2. 固件设置，其中关于固件的部分参考 ../firmware.md
3. 运行时的 sysfs 接口

## 参考
[Linux cpuidle 介绍](https://mp.weixin.qq.com/s/0LM25OrpFCCcokSCMv--Mg)

https://www.usenix.org/conference/osdi26/presentation/wang-yun

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
