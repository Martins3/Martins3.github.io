## [captive](https://github.com/tspink/captive)
- main
  - KernelLoader::create_from_file
  - KVM::create_guest

platform :
1. symbench
2. user
3. virt

- [ ] I think code in arch are auto generated, but how are called from
- [ ] captive only works on armv8, but no I don't know how to run it

binary translation

- 之所以出现了很多 unikernel 的代码，是因为这个 hypervisor 自己在 kvm 中间运行是需要二进制翻译的

## source code
- [ ] how engine loaded and executed ?

- Engine::load && Engine::install

`alloc_guest_memory(VM_PHYS_CODE_BASE, VM_CODE_SIZE, 0, (void *)0x670000000000);`

- [ ] how semihosting works ?

#### how unikernel works
为什么感觉 unikernel 像是不存在一样啊!

src/hypervisor/kvm/cpu.cpp : 的位置启动内核

X86DAGEmitter::cmp_lt is called by arm64-jit-chunk-7.cpp, it's used for emit x86 code.

dbt : x86-emitter.c  x86-value.c and encoder.c

- [ ] VirtualRegisterAllocator and ReverseAllocator


after some setup, we came here :
/home/maritns3/core/captive-project/captive/arch/common/cpu-block-jit.cpp

- GuestConfiguration
  - GuestCPUConfiguration
  - GuestMemoryRegionConfiguration
  - GuestDeviceConfiguration

- 在 x86 host 的一侧的启动位置，就是 ioctl RUN 了
- [x] 在 x86 guest 的一侧:
  - start_boot_core

```c
Attaching device gic-cpu-0 @ 8110000
Attaching device gic-distributor @ 8100000
Attaching device generic-timer-control @ 8030000
Attaching device pl031 @ 8040000
Attaching device pl011 @ 8020000
Attaching device (unknown) @ 8000000 // virtio bio 设备
Attaching device (unknown) @ 8010000 // virtio net 设备
```

- 所以 PL031 和 PL011 都是做什么的?
  - PL011 : 似乎是真正的串口设备
  - PL031 : 暂时不知道

```c
captive-platform.dts
11:             bootargs = "console=ttyAMA0 root=/dev/vda earlyprintk=pl011,0x08020000 consolelog=9 rw randomize_va_space=0 audit=0";
```
告诉 arm 内核，如果想要向串口输出，其 mmio 是多少，对于这些操作，其实翻译器是不在乎的。

arch/common/printf/printf.cpp:printf 的 0xfe 是如何解析的?
```c
    asm volatile("out %0, $0xfe\n" : : "a"(0));
```
在 handle_port_io 中间:

#### [ ] arch/common
- [ ] c++ 文件夹中间头文件，似乎是为了给这些函数提供一个标准的接口

#### [ ] 还是找不到 unikernel 的范围啊

#### 分析 keyboard 的流程
从 KVM_EXIT_MMIO 的位置开始。

#### 分析 GIC 设备的模拟实现
- [x] 如果 CPU 进行 GIC，如何?
- [x] 如果想要注入中断信息，如何?

从 PL011 调用到这里的:
```c
void KVMCpu::raise_guest_interrupt(uint8_t irq) {
  if (__sync_val_compare_and_swap(&per_cpu_data().isr, 0, 2) == 0) {
    // irq_raise.raise();

    KVMGuest &kvm_guest = (KVMGuest &)owner();

    uintptr_t ipp = (uintptr_t)per_cpu_data().interrupt_pending;
    if (ipp) {
      uint64_t *x = (uint64_t *)kvm_guest.vm_phys_to_host_virt(
          kvm_guest.vm_virt_to_vm_phys(ipp));
      *x = 1;
    }

    //*per_cpu_data().interrupt_pending = 1;
  }
}
```
- [ ] guest 对于中断的检测为什么不是靠 kvm 来实现?
- [ ] 到 guest 那一侧看看这个这个变量的检测!
- [ ] 到 Linux 源码检查 PL011 是如何使用的，写入一个字符然后靠中断通知搞定了 ?
- [ ] ArmCpuIRQController 和 GIC 的关系是什么?

- [ ] src/gic.c 似乎处理 gic 的 io，但是这些工作为什么不放到 guest 中间处理?
  - [ ] 恐怕是因为无法捕获这些 io 指令吧？

#### block device 如何处理的
- guest 的 virtio 实现？
  - 糟糕，从 captive/captive-platform.dts 看，其让 guest 是 virtio 设备，然后 host 实现
  - 我感觉这是一个更好的方案，因为这样内核中间很多的代码都是让 guest 实现的

- [ ] 但是，从 virtio 到具体的驱动，我的天啊!
  - 看看 includeOS 的实现方式吧!

## problem
- [x] 100% CPU
  - 这应该不是 bug，而是使用 poll 的原因

- [ ] why so many code about devices ?

实在是想不通，对于各种设备访问，captive 是如何模拟的 ?
  - xqm 是如何实现的，我也是很好奇的

## 文章阅读记录
- [ ] 2.3.2 Translation : 建立 DAG 实验部分。
- [ ] 2.7 Virual Memory Management 的划分为上下两个部分
