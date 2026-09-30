# Rust VMM + C guest

这个示例直接打开 `/dev/kvm`，由 Rust 创建一台只有一个 vCPU、2 MiB RAM
的虚拟机，执行用 C 编写的裸机程序。适合修改 guest 指令、观察 KVM
exit，或者继续添加自己的 KVM 实验。

## 构建与运行

需要 Linux x86_64、可读写的 `/dev/kvm`、Rust 1.85 或更新版本、GCC、GNU binutils
和 Make。首次 Cargo 构建需要下载依赖。默认 guest 不依赖 libc、操作系统、BIOS 或
QEMU。

在本目录执行：

```sh
make run
```

guest 输出到 stdout，VMM 状态和错误输出到 stderr。预期结果：

```text
Hello from C guest!
BSS zeroed; result = 42
mini-kvm: guest halted, guest_main returned 0
```

也可以分别构建、运行，或指定其他按相同布局链接的平坦镜像：

```sh
make
./mini-kvm.out guest.out
make check
make clean
```

`make check` 执行 Rust 格式检查、Clippy 和带 10 秒超时的真实 KVM 运行；需要
`timeout` 命令。`make run` 本身不限制 guest 的运行时间，循环实验可用 Ctrl-C
结束。

产物包括：

| 文件            | 用途                                 |
| --------------- | ------------------------------------ |
| `mini-kvm.out`  | Rust VMM                             |
| `guest.elf.out` | 保留符号和调试信息的 ELF，用于反汇编 |
| `guest.out`     | 从 ELF 导出的平坦镜像，作为 VMM 输入 |

Cargo 内部产物保留在已忽略的 `target/`，对外产物使用 `.out` 后缀，C 中间文件使用
`.o` 后缀。可用 `make CC=/usr/bin/gcc OBJCOPY=/usr/bin/objcopy` 指定工具。

## 从哪里改代码

- `guest/main.c` 中的 `guest_main()` 是 C 实验入口，返回 0
  表示成功。`guest_putc()`、`guest_puts()` 和 `guest_put_u64()` 提供基本输出。
- `guest/start.S` 中的 `_start` 只有调用 C 所需的入口代码：清除方向标志、调用
  `guest_main()`、保留 EAX 返回值并执行 `hlt`。Rust 预先设置 16
  字节对齐的栈，`call` 之后满足 x86_64 SysV ABI。
- `src/main.rs` 中的 `execute()` 创建 VM，`run()` 处理退出；添加端口或 MMIO
  实验时从 `run()` 扩展。
- `src/x86.rs` 中的 `setup()` 配置 CPUID、GDT、页表和寄存器；`src/memory.rs`
  中的 `GuestMemory` 管理 host 内存映射。
- `guest/linker.ld` 固定入口和镜像布局，并检查包含 BSS 在内的 guest
  不会覆盖预留栈。

例如修改 `calculate()` 后执行 `make run` 即可运行新代码。默认 `guest_main()`
会检查结果是否为
42；改变计算时也要调整预期值。可用以下命令检查实际指令和加载布局：

```sh
objdump -dS guest.elf.out
readelf -lW guest.elf.out
```

## KVM 调用流程

1. `Kvm::new()` 打开 `/dev/kvm`，检查 `KVM_GET_API_VERSION` 和
   `KVM_CAP_USER_MEMORY`。
2. `mmap` 分配清零且页对齐的 host RAM，加载平坦镜像；`KVM_CREATE_VM` 创建
   VM，`KVM_SET_USER_MEMORY_REGION` 把这段 host 内存注册为 GPA 从 0 开始的 slot
   0。
3. `KVM_SET_TSS_ADDR` 预留 Intel KVM 所需的 TSS 地址范围，`KVM_CREATE_VCPU` 创建
   vCPU。`kvm-ioctls` 在内部映射 `kvm_run`。
4. `KVM_GET_SUPPORTED_CPUID` / `KVM_SET_CPUID2` 设置 CPU 能力；在 guest RAM
   中写入 GDT 和页表，再用 `KVM_SET_SREGS`、`KVM_SET_REGS` 直接建立 64
   位执行环境。
5. `KVM_RUN` 执行 guest。`outb` 到 `0xe9` 产生 `VcpuExit::IoOut`，Rust 写入
   stdout，然后再次进入 `KVM_RUN` 完成该 I/O 并继续执行。
6. C 返回后，`hlt` 产生 `VcpuExit::Hlt`；VMM 读取 EAX 中的 C 返回值。0 对应 host
   退出码 0，任何非零 guest 返回值或 VMM 错误对应 host 退出码 1，并打印原因。

`0xe9` 是这个示例约定的调试输出端口，不实现 UART。未支持的 I/O、MMIO、shutdown
和其他退出均打印退出信息并失败；`KVM_RUN` 被信号打断且返回 `EINTR`
时重试。vCPU、VM、RAM 按此顺序释放，确保 KVM 使用内存期间映射始终有效。

参考：[Linux KVM API](https://docs.kernel.org/virt/kvm/api.html)、[kvm-ioctls](https://docs.rs/kvm-ioctls/0.25.0/kvm_ioctls/)。

## 内存与 CPU 状态

| GPA                    | 内容                                           |
| ---------------------- | ---------------------------------------------- |
| `0x1000`               | PML4                                           |
| `0x2000`               | PDPT                                           |
| `0x3000`               | PD，使用一个 2 MiB 大页                        |
| `0x5000`               | GDT：空描述符、64 位代码段、数据段             |
| `0x10000` 起           | `_start`、C 代码、只读数据、已初始化数据和 BSS |
| `[0x1f0000, 0x200000)` | 64 KiB 栈，向低地址增长                        |

GVA `[0, 2 MiB)` 恒等映射到 GPA
`[0, 2 MiB)`，整个区域可读、可写、可执行。入口地址和栈边界同时出现在
`src/x86.rs` 和 `guest/linker.ld`，修改时必须保持一致。Intel KVM 的 TSS 另使用
GPA `[0xfffbd000, 0xfffc0000)`，不与 guest RAM 重叠。

VMM 设置 CR0.PE/PG、CR4.PAE、EFER.LME/LMA，CS.L=1，CR3 指向 PML4，RIP 指向
`_start`。CPU 直接从 long mode 开始，不经过实模式启动过程。RFLAGS.IF=0，不配置
IRQ chip 或异常处理程序。

## C guest 的边界

这是 freestanding C 环境：没有 `printf`、`malloc`、系统调用和动态链接。编译禁用
PIE/PIC、栈保护、red zone、浮点/SIMD；增加代码时若编译器生成 `memcpy`
等运行库调用，需要自行提供实现。固定的 64 KiB 栈没有保护页。

匿名 RAM 初始为零，因此未包含在平坦镜像中的 BSS
也为零。加载器拒绝空镜像、超大镜像和 ELF 文件；平坦镜像没有入口、BSS
或栈元数据，只保证配套链接脚本产生的布局。其他镜像也必须遵守这个约定。

不安装 IDT 或异常处理程序，非法指令、缺页等 guest 异常可能最终表现为 triple
fault / `Shutdown`。第一版不提供 Linux 启动、多
vCPU、磁盘、网络和中断设备；后续实验可从当前创建流程和 exit 循环直接扩展。

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
