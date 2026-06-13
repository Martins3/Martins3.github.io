# Level 1 : stop continue
## 基本使用

## 代码流程

- [ ] `vmstate_register_ram`
    - 本来以为是放到某一个 list 中的，但是并没有


savevm 中 `load_snapshot` 和 `save_snapshot` 可以和 migration 共用
- `hmp_loadvm`
    - `load_snapshot` ：似乎是从一个 block driver 中 load 的 snapshot，TODO 为什么不是直接读取一个文件，而是搞的这么复杂啊
        - `bdrv_all_has_snapshot`
        - [ ] 类似的还有各种操作，但是最后的结果其实只是为了获取一个 QEMUFile
        - `qemu_loadvm_state`
            - `qemu_loadvm_state_header`
                - 检查 vmfile `QEMU_VM_FILE_MAGIC`
        - `qemu_loadvm_state_main`

- `hmp_savevm`
    - `save_snapshot`
        - `qemu_savevm_state` ：TODO 在 load 中，这个函数就是开始和 migration 开始共用，但是 save 是下一个函数才开始的
            - `qemu_savevm_state_iterate`

## [ ]  为什么 savevm 和 postcopy 有关系哇
似乎主要是 postcopy 的一些通信的操作

## 细节分析

### inflight block io 和网络 io 都是如何暂停的

如果 io engine 在 QEMU 内部，例如普通的 virtio-blk ，需要等 host 的 io 返回吗?

如果 io engine 在 QEMU 外部，例如 SPDK 的，如何处理的

### 下发暂停命令之后，vCPU thread 如何结束的

### 当重新启动虚拟机，那些 vCPU thread 是如何启动的?

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_vcpu_ioctl


hmp 命令都是 `hmp_` 开头的，容易找到:

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - handle_hmp_command_exec
                              - hmp_cont
                                - qmp_cont
                                  - resume_all_vcpus
                                    - resume_all_vcpus
                                      - cpu_resume
                                        - qemu_cpu_kick
                                          - qemu_cpu_kick
                                            - cpus_kick_thread

没想到啊，居然用的是这个方法:
```c
    int err = pthread_kill(cpu->thread->thread, SIG_IPI);
```

当 vCPU 停住的时候，代码的结构在这里:

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - qemu_wait_io_event
          - qemu_cond_wait_impl
            - pthread_cond_wait@@GLIBC_2.3.2
              - __futex_abstimed_wait_common



暂停和启动就是普通的 flag 就可以解决:

```c
void cpu_pause(CPUState *cpu)
{
    if (qemu_cpu_is_self(cpu)) {
        qemu_cpu_stop(cpu, true);
    } else {
        cpu->stop = true;
        qemu_cpu_kick(cpu);
    }
}

void cpu_resume(CPUState *cpu)
{
    cpu->stop = false;
    cpu->stopped = false;
    qemu_cpu_kick(cpu);
}
```


- qmp_cont
  - vm_start
    - vm_prepare_start
      - vm_state_notify
        - kvmclock_vm_state_change
    - resume_all_vcpus

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
