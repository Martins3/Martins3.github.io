## `async_pf`
Asynchronous page fault is a way to try and use guest vcpu more efficiently by allowing it to execute other tasks while page is brought back into memory[1].


## host 中的路径
路径1:
- `kvm_mmu_do_page_fault`
  - `kvm_tdp_page_fault`
    - `direct_page_fault`
      - `try_async_pf`
        - `kvm_arch_setup_async_pf`
          - `kvm_setup_async_pf` : 在其中初始化一个 workqueue 任务，并且放在队列中间

路径2: 这个路径
- `handle_exception_nmi`
  - `kvm_handle_page_fault` : 入口函数, 靠近 exit handler
    - `kvm_mmu_page_fault`
      - `kvm_async_pf_task_wait_schedule`

## guest 中的流程
- asm_sysvec_kvm_asyncpf_interrupt 就是 guest 接受到 host 的信息说可以了。

- init_hypervisor_platform 中初始化，然后在

在 guest 中初始化这个:
```txt
#0  kvm_guest_init () at arch/x86/kernel/kvm.c:811
#1  0xffffffff837ee418 in setup_arch (cmdline_p=cmdline_p@entry=0xffffffff82e03f18) at arch/x86/kernel/setup.c:1282
#2  0xffffffff837e19a8 in start_kernel () at init/main.c:963
#3  0xffffffff81000145 in secondary_startup_64 () at arch/x86/kernel/head_64.S:358
```
- [ ] 这个函数为什么 host

- `KVM_FEATURE_ASYNC_PF_INT` : guest 是通过 cpuid 获取的
  - 这些 feature 应该都是有的


- [ ] 和 KVM_FEATURE_ASYNC_PF 的关系是什么？ KVM_FEATURE_ASYNC_PF 似乎根本没用啊

## 资料
- [ ] https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/03/24/kvm-async-page-fault
1. 需要修改内核kvm 外面的代码 ? 不然怎么来识别从 host inject 的
2. 内核如何调度 host 的另一个 task 过来运行的
> 算是最清楚的教程了，TODO
- [ ] https://lwn.net/Articles/817239/

- [https://lwn.net/Articles/845473/](aarch64: Support Asynchronous Page Fault)
  - 这个 patch 描述的比较清楚了

## 为什么需要这种设计
因为 vCPU 发生 page fault 了，被切换走，然后 Host 从磁盘中请求内容，然后 host 就很尴尬:
- 不能继续执行 vCPU 线程了，因为如果开始执行，就需要保证该页已经准备好。
- host 侧没有什么任务需要执行。

基本的设计:
- host 需要告诉 Guest ，正在进行 apf ，可以干其他事情。
- Guest 需要接受到一个

## 基本代码流程

host 的流程:
```txt
@[
    kvm_faultin_pfn+1
    direct_page_fault+774
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 275741
```

- direct_page_fault
  - kvm_faultin_pfn
    - `__kvm_faultin_pfn`
      - kvm_make_request(KVM_REQ_APF_HALT, vcpu);

```txt
  kvm:kvm_try_async_get_page                         [Tracepoint event]
  kvm:kvm_async_pf_completed                         [Tracepoint event]
  kvm:kvm_async_pf_not_present                       [Tracepoint event]
  kvm:kvm_async_pf_ready                             [Tracepoint event]
  kvm:kvm_async_pf_repeated_fault                    [Tracepoint event]
```

sudo bpftrace -e 'tracepoint:kvm:kvm_try_async_get_page { @[kstack] = count(); }'

## 如何打开 page fault 机制
似乎我的 Guest 内核有问题?

t -r kvm_faultin_pfn 的返回值总是 0

- `__kvm_faultin_pfn` 中调用不到 kvm_can_do_async_pf 中去


- 猜测是 hva_to_pfn_slow 中，因为没有 memcg，所以也根本不存在这个需求了
  - 但是还是不行，应该深入理解下 gup 的含义
  - 而且我们见过内存充足的上下文。

- `__kvm_faultin_pfn`
  - `__gfn_to_pfn_memslot`
    - hva_to_pfn
      - hva_to_pfn_slow

如果不是使用文件
```txt
@[
    get_user_pages_unlocked+5
    hva_to_pfn+268
    kvm_faultin_pfn+146
    direct_page_fault+774
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 206272
```

使用文件作为 backend 的时候，这个 gup 才让进入到下面的判断，实际上，async page fault 的情况很少:
```txt
@[
    kvm_can_do_async_pf+5
    kvm_faultin_pfn+181
    direct_page_fault+774
    kvm_mmu_page_fault+276
    vmx_handle_exit+374
    kvm_arch_vcpu_ioctl_run+3286
    kvm_vcpu_ioctl+629
    __x64_sys_ioctl+139
    do_syscall_64+60
    entry_SYSCALL_64_after_hwframe+114
]: 280751
```

不知道 kvm_can_do_async_pf 的哪一个判断失败的

- `__kvm_faultin_pfn`
		- kvm_make_request(KVM_REQ_APF_HALT, vcpu);

- 将任务发送到 workqueue

导致 vcpu 重新进入的时候 ：
- vcpu_enter_guest
  - vcpu->arch.apf.halted = true;


## 分析 KVM_FEATURE_ASYNC_PF_INT

- sysvec_kvm_asyncpf_interrupt
  - kvm_async_pf_task_wake
  - wrmsrl(MSR_KVM_ASYNC_PF_ACK, 1);
