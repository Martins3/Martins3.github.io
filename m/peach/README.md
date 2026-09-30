## 代码来自于
https://github.com/pandengyang/peach/blob/master/guest/Makefile

## virtme 调试结果（2026-09-24）

可以用 virtme 调试，已实际跑通嵌套 VMX：物理机 KVM 是 L0，virtme
Linux 和 peach 模块是 L1，`guest.h` 的 17 字节实模式程序是 L2。
virtme 提供共享文件系统和调试环境；VMX 指令的嵌套支持来自 L0 KVM。

本次使用 i9-13900K、`kvm_intel.nested=Y`、QEMU `-cpu host`，L1 内核是
`7.2.0-00001-geb5a10dc0e00-dirty`，独立 VM 名称为 `peach-debug`，2 vCPU、2 GiB。
修改保存在 `pearch` worktree，用户提供的 `simplefs` worktree 未修改。

原版有两个已复现的阻塞点：

1. `handle_vmexit()` 在 inline asm 中修改 RSP/RBP，跳到
   `peach_ioctl()` 内部的 `shutdown` 标签。当前 objtool 无法为这段控制流
   生成可靠的栈展开信息，构建报 `unknown CFA base reg -1`。
2. 在临时副本中仅跳过 objtool 以观察原始 VMX 路径后，virtme 日志显示
   `IA32_VMX_BASIC=0x01d8100011e57ed0`，实际 revision ID 为 `0x11e57ed0`。
   原版将 VMXON/VMCS revision 写死为 `1`，`VMXON` 返回失败后却继续执行
   `VMCLEAR`，在 `peach_ioctl()` 中触发 `invalid opcode` / `#UD`。
   该临时构建还产生了 return thunk 警告，不能作为正常构建方案。

现在 `peach_enter()` 在汇编中保存/恢复调用者寄存器，以当前内核栈作为
HOST_RSP，每次 VM-exit 正常返回 `run_guest()`；使用 `SYM_FUNC_START/END`、
`UNWIND_HINT_SAVE/RESTORE`、`ENDBR` 和 `RET`，正常构建无需跳过 objtool。
VMCS 使用内核字段名，revision 和控制位从能力 MSR 推导。VMX 区间禁止
抢占和 IRQ；失败时停止执行后续 VMX 指令，恢复 CR4 并释放内存。
EPT 使用对齐页面，并在运行前执行 INVEPT，支持反复运行。

实际成功日志：

```text
peach: VMLAUNCH -> VM-exit reason=0xa rip=0x3 ax=0x0 bx=0x0 cx=0x0
peach: VMRESUME -> VM-exit reason=0xc rip=0x10 ax=0x4348 bx=0x4541 cx=0xe050
peach: run result=0
```

`0xa` 是 CPUID，`0xc` 是 HLT。`run_guest()` 为 CPUID 填入 AX/BX/CX，
推进 RIP 后执行 VMRESUME；L2 做三次 16 位减法，HLT 时校验上述结果。
外部中断退出会返回 `EAGAIN`，用户程序短暂等待后重新运行。

验证包括正常构建无 objtool 警告、CPU 0/CPU 1 各 100 次完整运行、模块
卸载再加载、未知 ioctl 返回 `ENOTTY`。在临时副本中再次把 VMXON revision
改成 `1`，现在正确返回 `EIO`，没有 Oops；换回正确模块后又完成 100 次运行。
失败后的模块和设备节点也均可正常清理。

本地证据保存在被 git 忽略的 `debug.out/`：`baseline-dmesg.log`、
`fixed-test.log`、`fixed-dmesg.log`、`bad-revision-test.log` 和
`after-failure-test.log`。NMI 退出转交路径已实现，但未做定向 NMI 注入验证。
这里只运行内置的固定程序，不是可运行任意 guest 的通用 hypervisor。

## 编译和复测

物理机只编译，不加载模块：

```bash
make -C /home/martins3/data/vn/.worktrees/pearch/m/peach
```

`KERNEL_SOURCE` 默认是 `~/data/kernel/default`，必须与 virtme 的运行内核
匹配。独立 VM 的 `config.ini` 使用 `virtme = 1`、`vsock = 1`，并在
`cmdline` 加上 `module_blacklist=kvm,kvm_intel`，避免 L1 KVM 抢占 VMX。
使用 collei 的 `run` / `ssh_auto` 操作获取实际连接方式；本次 VM 的 vsock CID
为 1134。测试需要 root，直接使用 guest 的 root SSH；不要依赖共享 rootfs
中的 sudo 文件权限。

下面命令在物理机执行，将产物放进 guest 自己的 `/tmp`，避免 virtme overlay
隐藏重新编译后的文件：

```bash
cd /home/martins3/data/vn/.worktrees/pearch/m/peach
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  -o 'ProxyCommand=/home/martins3/.nix-profile/bin/socat - VSOCK-CONNECT:1134:22' \
  root@virtme 'mkdir -p /tmp/peach'
scp -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  -o 'ProxyCommand=/home/martins3/.nix-profile/bin/socat - VSOCK-CONNECT:1134:22' \
  peach.ko peach-user.out run-virtme.sh root@virtme:/tmp/peach/
ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
  -o 'ProxyCommand=/home/martins3/.nix-profile/bin/socat - VSOCK-CONNECT:1134:22' \
  root@virtme 'bash /tmp/peach/run-virtme.sh'
```

`run-virtme.sh` 检查 virtme 环境，加载模块、创建设备节点、执行 100 次测试，
退出时删除节点并卸载模块。模块自身也拒绝在没有 VMX 的环境或非 KVM guest
中加载。成功输出为 `PASS: 100 CPUID -> HLT runs, N interrupted retries`。

## 这里的代码，也是有 static call

和 APIC 是一样的实现原理吗?
```c
static u64 native_steal_clock(int cpu)
{
	return 0;
}

DEFINE_STATIC_CALL(pv_steal_clock, native_steal_clock);
DEFINE_STATIC_CALL(pv_sched_clock, native_sched_clock);

void paravirt_set_sched_clock(u64 (*func)(void))
{
	static_call_update(pv_sched_clock, func);
}
```

## 也许这个东西会更加好
https://seiya.me/blog/riscv-hypervisor

[转][译] 100 行 C 代码创建一个 KVM 虚拟机（2019） - 李睿的文章 - 知乎
https://zhuanlan.zhihu.com/p/701258802
