# ARM KVM 的大致代码流程
> 尽可能的看看熟悉的部分

- [ ] 天高气爽阅码疾：一日看尽虚拟化（上）: https://mp.weixin.qq.com/s/CWqUagksabj4kDFQhTlgUA
  - 应该将这个作者的其他内容也好好找一找。

## 资源

- [ ] https://www.usenix.org/system/files/conference/atc17/atc17-dall.pdf
- [ ] https://calinyara.github.io/technology/2019/11/03/armv8-virtualization.html
- [ ] https://openeuler.org/zh/blog/yorifang/2020-10-24-arm-virtualization-overview.html

## 读读代码
好吧，存在 VHE 和 none VHE 两种方式。

- kvm_arch_vcpu_ioctl_run
  - `__kvm_vcpu_run` : vhe 和 nvhe 各有一份的
    - `__guest_enter` : arch/arm64/kvm/hyp/entry.S
  - handle_exit
   - handle_trap_exceptions
    - kvm_get_exit_handler
    - arm_exit_handlers : 所有的 handler 入口
      - kvm_handle_guest_abort : mmu.c 处理 tdp 的映射问题, 这些和 MIPS 中间的内容非常的对称

## [内存虚拟化](https://www.cnblogs.com/LoyenWang/p/13943005.html)

## [中断虚拟化](https://www.cnblogs.com/LoyenWang/p/14017052.html)
整个 kvm 只有 17000 行，其中 kvm/vgic 下有 7000 行

- [ ] 分析的相当不错，但是只能理解其中部分内容
