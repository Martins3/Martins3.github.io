## `async_pf`
Asynchronous page fault is a way to try and use guest vcpu more efficiently by allowing it to execute other tasks while page is brought back into memory[1].


## host 中的路径
路径 1:
- `kvm_mmu_do_page_fault`
  - `kvm_tdp_page_fault`
    - `direct_page_fault`
      - `try_async_pf`
        - `kvm_arch_setup_async_pf`
          - `kvm_setup_async_pf` : 在其中初始化一个 workqueue 任务，并且放在队列中间

路径 2: 这个路径
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
1. 需要修改内核 kvm 外面的代码 ? 不然怎么来识别从 host inject 的
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

如果不是使用文件的时候:
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

一般来说 host 的执行流程:
- `__kvm_faultin_pfn`
    - kvm_find_async_pf_gfn : 这是在查询缓存吗?
		- kvm_make_request(KVM_REQ_APF_HALT, vcpu);
    - kvm_arch_setup_async_pf : 启动一个 workqueue 来处理
      - kvm_setup_async_pf
        - async_pf_execute : 这是实际上执行的内容
          - get_user_pages_remote : 完成 page fault 的过程
          - kvm_arch_async_page_present : 通知 guest 事情搞定了
          - `__kvm_vcpu_wake_up` : 执行完成之后，让 vCPU 开始启动

guest 的执行流程:
- kvm_handle_async_pf
  - `__kvm_handle_async_pf`
    - kvm_async_pf_task_wait_schedule : 被 swapout 的 page，所以睡眠

## 分析 KVM_FEATURE_ASYNC_PF_INT

- sysvec_kvm_asyncpf_interrupt
  - kvm_async_pf_task_wake
  - wrmsrl(MSR_KVM_ASYNC_PF_ACK, 1);

```diff
History:        #0
Commit:         2635b5c4a0e407b84f68e188c719f28ba0e9ae1b
Author:         Vitaly Kuznetsov <vkuznets@redhat.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    2020年05月25日 星期一 22时41分20秒
Committer Date: 2020年06月01日 星期一 16时26分07秒

KVM: x86: interrupt based APF 'page ready' event delivery

Concerns were expressed around APF delivery via synthetic #PF exception as
in some cases such delivery may collide with real page fault. For 'page
ready' notifications we can easily switch to using an interrupt instead.
Introduce new MSR_KVM_ASYNC_PF_INT mechanism and deprecate the legacy one.

One notable difference between the two mechanisms is that interrupt may not
get handled immediately so whenever we would like to deliver next event
(regardless of its type) we must be sure the guest had read and cleared
previous event in the slot.

While on it, get rid on 'type 1/type 2' names for APF events in the
documentation as they are causing confusion. Use 'page not present'
and 'page ready' everywhere instead.

Signed-off-by: Vitaly Kuznetsov <vkuznets@redhat.com>
Message-Id: <20200525144125.143875-6-vkuznets@redhat.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```
看上去为来方式 exception ，所以专门做了一个新的入口。
