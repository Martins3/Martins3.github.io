# hvc
## 第一个问题 : 为什么我们似乎是没有指定 bus 的
• 它们是 “控制器”和“控制器上的端口” 的关系。

   参数                      作用
  ━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   -device virtio-serial     创建 virtio 串口控制器，提供一条可挂多个端口的总线
  ────────────────────────  ────────────────────────────────────────────────────
   -device virtserialport    在这条总线上创建一个通信端口
  ────────────────────────  ────────────────────────────────────────────────────
   -chardev ...              定义端口在宿主机侧连接的后端，例如 Unix socket

  一个控制器可以挂多个 virtserialport，各端口可以连接不同的后端：

  virtio-serial 控制器
  └── virtio-serial 总线
      ├── virtserialport → chardev → Guest Agent socket
      └── virtserialport → chardev → 其他应用 socket

### 到底谁在用
/dev/hvc0 和 /dev/vport6p0 会使用
```txt
  lspci -s 00:08.0 -v
00:08.0 Communication controller: Red Hat, Inc. Virtio console
        Subsystem: Red Hat, Inc. Device 0003
        Flags: bus master, fast devsel, latency 0, IRQ 20
        I/O ports at c080 [size=64]
        Memory at fe0c6000 (32-bit, non-prefetchable) [size=4K]
        Memory at 38000001c000 (64-bit, prefetchable) [size=16K]
        Capabilities: <access denied>
        Kernel driver in use: virtio-pci
        Kernel modules: virtio_pci

```

## 第二个问题: 我们是可以设置多个 virtio device 吗?


# 什么情况，virtioconsole 是 virtio-serial 下的一种?


• -device virtconsole 是 virtio 控制台端口，和 virtserialport 一样，挂在 virtio-serial 控制器提供的总线上。

  virtio-serial 控制器
  └── virtio-serial 总线
      ├── virtconsole    → chardev → 控制台终端
      └── virtserialport → chardev → 应用通信通道

  它们的区别主要在 guest 如何使用：

   设备              用途                                       Linux guest 中的典型节点
  ━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   isa-serial        传统 UART 串口，可作控制台                 /dev/ttyS0
  ────────────────  ─────────────────────────────────────────  ───────────────────────────────────────
   virtconsole       virtio 控制台，可承载内核日志、登录终端    /dev/hvc0
  ────────────────  ─────────────────────────────────────────  ───────────────────────────────────────
   virtserialport    通用字节流通信，如 Guest Agent             /dev/vport*、/dev/virtio-ports/<name>

  例如，把 virtio 控制台连接到宿主终端：

  -device virtio-serial,id=vser0
  -chardev stdio,id=con0
  -device virtconsole,bus=vser0.0,chardev=con0

  Linux guest 需要支持 CONFIG_VIRTIO_CONSOLE；要输出内核控制台日志，使用 guest 内核参数 console=hvc0；要交互登录，还需要在 hvc0
  上启动 getty。

  从本地源码看，virtconsole 直接继承 virtserialport，并设置 is_console = true，见 hw/char/virtio-console.c:255。控制器会把这
  个“控制台端口”标志告诉 guest，让驱动按控制台方式处理它。


### 所以，virtconsole 有了特殊设置，才可以承担 内核日志  登录终端 吗?

   virtconsole       virtio 控制台，可承载内核日志、登录终端    /dev/hvc0


# 居然如此?


• 如果限定为 x86 的 pc / q35 machine，可以确认：-serial 最终创建的就是 isa-serial，使用同一套设备实现。 这不是仅仅行
  为相似，源码中确实创建了同一种对象。

  调用关系是：

  -serial <backend>
    → serial_parse()：记录字符后端
    → PC 初始化：serial_hds_isa_init()
    → serial_isa_init()
    → isa_new(TYPE_ISA_SERIAL)
    → 设置 index、chardev，然后 realize

  关键代码见 serial_isa_init() (hw/char/serial-isa.c:163)：

  isadev = isa_new(TYPE_ISA_SERIAL);
  dev = DEVICE(isadev);
  qdev_prop_set_uint32(dev, "index", index);
  qdev_prop_set_chr(dev, "chardev", chr);
  isa_realize_and_unref(isadev, bus, &error_fatal);

  因此，对于单个 COM1，在后端和设备属性相同的情况下：

  -serial stdio

  和：

  -serial none
  -chardev stdio,id=char0
  -device isa-serial,index=0,chardev=char0

  使用的是同一个 UART 设备模型，guest 侧的串口功能相同。

  但仅凭“x86”不能保证：-serial 本身只登记后端，由 machine 决定如何使用。例如 microvm 只有启用 isa-serial 属性时才创
  建对应设备，见 hw/i386/microvm.c:274。所以准确的限定条件是 machine 类型和配置。


## 似乎 virtioconsole 和 virtio-port 的结果基本上可以合并了

源码把这两步分得很明确。

QEMU 的 /home/martins3/data/qemu/hw/char/virtio-console.c 中，virtconsole_class_init() 设置：

k->is_console = true;

随后 /home/martins3/data/qemu/hw/char/virtio-serial-bus.c 向 guest 发送：

VIRTIO_CONSOLE_CONSOLE_PORT

Linux 的 /home/martins3/data/kernel/linux-drm/drivers/char/virtio_console.c 收到这个消息，才调用：

init_port_console()
  → hvc_alloc()
  → 接入 HVC 控制台框架

普通 virtserialport 没有这一步，提供的是 /dev/vportXpY 的 read/write 字符设备接口；控制台端口则接入
/dev/hvcN。VirtioSerial 说明 (https://fedoraproject.org/wiki/Features/VirtioSerial)

因此对应关系是：

 QEMU 端口类型     guest 启动参数                 能否通过 HVC 解析 SysRq
━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 virtserialport    console=vport0p1               不能，没有对应的 console 驱动
────────────────  ─────────────────────────────  ────────────────────────────────────────────
 virtserialport    console=hvc0                   不能把这个普通端口转换为 HVC
────────────────  ─────────────────────────────  ────────────────────────────────────────────
 virtconsole       console=hvc0，且它对应 hvc0    可以，发送 Ctrl+O,w，并满足 SysRq 开关条件

另外，实现 console 输出能力本身也不自动带来 SysRq 能力。输入路径还必须显式解析触发序列并调用
handle_sysrq()；HVC 已经实现了这一部分。

所以你的思路可以通过**把目标端口配置成 virtconsole，再选择 console=hvcN**来实现，
而不是给普通 vport 加一个启动参数。

TODO 继续调查下 qemu 代码的实现


## serial
https://qemu-project.gitlab.io/qemu/interop/qemu-ga-ref.html

qga 似乎可以同时使用 socat 和 qmp shell 的
```txt
function cmd_monitor() {
	items=(
		"qmp" "shell" "qga" "main"
	)
	resource=$(printf "%s\n" "${items[@]}" | fzf)
	case "$resource" in
		qmp)
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu"/qmp
			;;
		shell)
			qmp_shell=${QEMU_DIR}/scripts/qmp/qmp-shell
			$qmp_shell "$vm_dir/$which_qemu"/qmp-shell
			;;
		qga)
			# 原来 qga 可以执行这么多命令啊
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu"/qga.sock
			;;
		main)
			# 当 gdb 调试的时候，使用 socket 来连接
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu"/main.sock
			;;
		*)
			printf "%s\n" "${items[@]}"
			;;
	esac

}
```
## 原来 hvc0 是 legacy 啊
printk: legacy console [hvc0] enabled


https://projectacrn.github.io/latest/developer-guides/hld/virtio-console.html

## 两个源码
/home/martins3/data/kernel/linux-drm/drivers/char/virtio_console.c
和 hyperconsole 什么关系啊?

## 为什么 firecracker 的 ttyS0 去支持 serial 而不是 virtconsole

没有非常清晰的答案，

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
