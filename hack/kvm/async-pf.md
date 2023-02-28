## `async_pf`
Asynchronous page fault is a way to try and use guest vcpu more efficiently by allowing it to execute other tasks while page is brought back into memory[1].


路径1:
- `kvm_mmu_do_page_fault`
  - `kvm_tdp_page_fault`
    - `direct_page_fault`
      - `try_async_pf`
        - `kvm_arch_setup_async_pf`
          - `kvm_setup_async_pf` : 在其中初始化一个 workqueue 任务，并且放在队列中间

路径2:
- `kvm_handle_page_fault` : 入口函数, 靠近 exit handler
  - `kvm_mmu_page_fault`
  - `kvm_async_pf_task_wait_schedule`


- [ ] https://terenceli.github.io/%E6%8A%80%E6%9C%AF/2019/03/24/kvm-async-page-fault
1. 需要修改内核kvm 外面的代码 ? 不然怎么来识别从 host inject 的
2. 内核如何调度 host 的另一个 task 过来运行的
> 算是最清楚的教程了，TODO
- [ ] https://lwn.net/Articles/817239/
