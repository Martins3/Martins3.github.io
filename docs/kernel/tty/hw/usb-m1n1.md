# asahi linux 如何调试

实际上，**Asahi 不是“没有串口”，而是 Mac 没有传统的串口插座。**他们既能通过
USB-C 引出真正的硬件 UART，也能用 **m1n1 的 USB 调试通道和虚拟
UART**。官方开发文档明确说：对大多数内核开发和逆向工作，后者已经足够；调试 KVM
等底层功能时才特别需要真实串口。:chatgpt-content-reference{index="0"}

## 1. 真正的硬件 UART：藏在 USB-C 接口里

以 Asahi 文档中的 M1 Mac 为例，SoC 的调试 UART 可以被引出到**特定 USB-C
接口，也就是用于 DFU 的那个接口**。

不过，它不是插一根普通 USB 线就会自动出现的 USB 串口。需要先通过 **USB-PD
的厂商自定义消息，VDM（Vendor Defined Message）**，让硬件切换引脚功能，把 UART
信号接到 USB-C 的引脚上；文档中的方案使用 SBU1/SBU2。UART 使用的是 **1.2 V
电平**。:chatgpt-content-reference{index="1"}

这里要区分：

> **USB-C 是连接器形状，不代表它的所有引脚此时都在传 USB
> 数据。这里传输的可以是真正的 UART 电信号。**

具体连接方式，可以使用带 USB-PD 控制和电平转换的调试板，例如文档中的 **Central
Scrutinizer**；也可以让另一台受支持的 Apple Silicon Mac 配合
`macvdmtool`，把两端端口配置为串口模式。后一种串口连接要求线缆包含所需引脚，普通
USB 2.0 充电线不行。:chatgpt-content-reference{index="2"}

所以，**真正需要做非常早期的 bring-up 时，他们并不缺硬件串口。**

## 2. 更方便的日常方案：m1n1 在 Linux 下面提供调试环境

m1n1 不只是引导程序，还包含硬件实验环境、远程控制协议和一个用于调试的
hypervisor。它的 proxy 模式能在 Linux 尚未启动时，把 Mac 作为 USB
设备连接到另一台开发机。:chatgpt-content-reference{index="3"}

开发机运行 Linux 时，通常会看到：

```text
/dev/ttyACM0    m1n1 的远程控制接口
/dev/ttyACM1    留给 hypervisor 的虚拟 UART
```

第一路供主机上的 Python 工具控制目标机器、加载内核等；第二路可以用串口终端查看
guest 的控制台输出。这是 **m1n1 自己提供的 USB 接口**，不是等目标 Linux
启动后再创建的 USB gadget。:chatgpt-content-reference{index="4"}

### Linux 的 USB 驱动还没工作，为什么能看到日志？

关键是：**调试时可以把 Linux 放到 m1n1 hypervisor 里面运行。**

以经典 M1 调试架构理解，就是 m1n1 占用 EL2，Linux 作为 guest 在下面运行；m1n1
提供虚拟 UART，并负责把输出送到开发机。这个 hypervisor 的目标不是把整台 Mac
的硬件全部模拟一遍，而是尽量让 guest
使用真实硬件，只在需要的地方进行拦截和调试。:chatgpt-content-reference{index="5"}

从 Linux 输出日志的角度，可以把路径理解成：

```text
Linux printk / earlycon
        ↓
虚拟 UART 的寄存器访问
        ↓
m1n1 截获、处理串口操作
        ↓
m1n1 的 USB 通道
        ↓
开发机的 /dev/ttyACM1
```

因此，**Linux 不需要先把自己的 USB
驱动、网络、用户空间启动起来，才能把早期日志送出去**。官方文档直接提供了“在 m1n1
hypervisor 下启动 Linux，并使用虚拟
UART”的开发流程。:chatgpt-content-reference{index="6"}

注意，这说的是**专门的 hypervisor 调试启动模式**，不是说正常安装的 Asahi
每次都必须作为 m1n1 的虚拟机运行；文档分别提供了直接启动内核和在 hypervisor
下启动内核的两条路径。:chatgpt-content-reference{index="7"}

## 3. 不仅能打印日志，还能从内核外面暂停和检查它

在 m1n1 hypervisor 模式下，开发者可以中断 guest
执行、检查执行上下文、查看带符号的调用栈、切换正在检查的 CPU，再恢复运行。例如
hypervisor 控制台有 `bt`、`ctx`、`cpu(1)`、`cont`
等命令；还可以启动调试服务器，让 GDB 或 LLDB
连接。:chatgpt-content-reference{index="8"}

这和“在 Linux 内核里面放一个调试器”有一个重要区别：

> **调试控制层在待调试内核之外，不需要依靠这个内核自己的调度器或用户空间来响应调试命令。**

但这不等于任何死机都能救回来：按这个架构推断，假如故障破坏了 m1n1
自身、它使用的内存或 USB
通路，这条调试链路也会失效。因此虚拟串口不能完全替代更简单的硬件
UART。:chatgpt-content-reference{index="9"}

### 逆向苹果硬件时，他们还会把 macOS 放进去运行

在受支持的机型和 macOS 版本组合中，m1n1 能运行
macOS，并**拦截、记录苹果驱动对真实硬件的访问**。这样就能观察寄存器访问、设备初始化流程，以及与协处理器之间的通信，而不是完全靠猜寄存器的意义。Asahi
的 DCP
显示控制器逆向就是这种方法的实际应用。:chatgpt-content-reference{index="10"}

所以它不仅解决了“怎么看到自己的 `printk`”，还解决了：

**“没有硬件手册时，怎样看到苹果自己的驱动到底对硬件做了什么？”**

## 4. 最早期启动和 KVM 调试怎么办？

**m1n1 自己还没把 USB 初始化好时，可以使用真实 UART，也可以利用固件已经准备好的
framebuffer。**Apple 的 iBoot 会把 framebuffer 地址等启动信息传给 m1n1；早期
Asahi 就直接向这块内存写像素来显示内容，并不要求先写好完整 GPU
驱动。:chatgpt-content-reference{index="11"}

**调试 KVM 则是虚拟串口方案的重要例外。**在上述 M1 模式中，m1n1 hypervisor
已占据 EL2，不能再把下面的 Linux 当成拥有同样 EL2 环境的裸机 KVM
主机。因此官方专门指出，调试 KVM 时可能无法使用这套 hypervisor
虚拟串口，需要真实串口方案。:chatgpt-content-reference{index="12"}

概括起来，Asahi 的方法是：**底层有隐藏的硬件 UART 兜底，日常用 m1n1 经 USB
提供虚拟串口和外部调试器，逆向时再利用 hypervisor 观察 macOS
对真实硬件的操作。**并不是只能等 Linux 启动成功以后，再通过 SSH 去调试。

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
