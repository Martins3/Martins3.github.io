# TCG
- [ ] 从 capstone 到 arm, 似乎对理解 binary 的构成还是很有帮助的呀!
  - [ ] 似乎在 gen_intermediate_code 中间都是手动分析编码的，和 capstone 没有任何关系啊

- [ ] linux-user 的部分似乎已经涉及到了 tcg 的处理了，看看 本一的工作吧

- [x] 测试一个 arm 吧

- [ ] /home/maritns3/core/qemu/accel/tcg 和 /home/maritns3/core/qemu/tcg 是什么关系 ?

- [ ] 如果没有 enable kvm 的话，x86_64 的机器上进行 x86_64 的翻译是怎么进行的 ？

- [ ] 用户态的翻译是不是不需要各种地址加速的处理啊

- [ ] 模拟 和 qemu 执行的上下文切换在哪里呀 ?

- [ ] load store 的 softmmu 的 walk 在哪里?

- [ ] 如果在 tb 的执行中间发生了越过 tb 的跳转，需要进行 tb_find 吗 ?

- [ ] 为什么二进制翻译需要考虑到内存一致性的问题

- [ ] 需要验证的流程:
  - [ ] tb 的靠什么组织的，linked list ?
  - [ ] 当 tb 可以实现代码的时候，

- [ ] 靠什么规则查询 tb ?

- [ ] tcg 定义的规则指令是什么 ?

- [ ] 二进制翻译从哪里开始的，是不是 linker 和 loader 的功能也是需要实现的，是直接调用本地的 linker 和 loader 还是需要存在配套的 linker 和 loader ?
  - [ ] 从 arm 之前的机器看，只是需要从 linker 和 loader 只是需要本地的就可以了，但是具体 qemu 为什么可以知道需要模拟动态链接，暂时不清楚啊。

- [ ] helper 函数到底是个什么概念 ?

## arm64

## load store
cpu_exec : 的两个调用者
* linux-user/aarch64/cpu_loop.c::cpu_loop
* cpus.c:tcg_cpu_exec : 支持单线程模拟和多线程模拟的方式
  * qemu_tcg_rr_cpu_thread_fn
  * qemu_tcg_cpu_thread_fn

- cpu_exec
  - tb_find
    - tb_lookup__cpu_state :
      - first search the `tb_jmp_cache` stored in `CPUState`
      - then call `tb_htable_lookup` to search all the translated and stored TB.
    - tb_gen_code : @todo 关注最后这个返回值是怎么组装起来的 ?
      - `get_page_addr_code` : get the `phys_pc` according to `pc`
        - [ ] 这里会进入到 cputlb.c 中间，如果这里也是需要进行 TLB 翻译的，和 gencode 时候的 TLB 翻译的关系是什么 ?
      - gen_intermediate_code : 在文件夹 target/arm 下面, 这个短短的函数用于选择到底是 arm 还是 arm64 的 TranslatorOps.
      - tcg_gen_code : 最终的代码会出现在 qemu/tcg/aarch64/tcg-target.inc.c 中间
        - tcg_reg_alloc_op
          - tcg_out_op
            - tcg_out_qemu_st
              - tcg_out_tlb_load : 这个应该是生成查询 TLB 的软件代码吗 ? 
              - tcg_out_qemu_st_direct : 直接进行 store 操作
      - tb_link_page : set this TB's related physical page
  - cpu_loop_exec_tb
    - cpu_tb_exec

从 gen_intermediate_code 的位置分析, target 表示需要被翻译的平台, 所以需要被分析编码
- 对于 arm64，其具体代码在 qemu/target/arm/translate-a64.c
- aarch64_tr_translate_insn
  - disas_a64_insn
    - disas_data_proc_reg
      - disas_adc_sbc
        - gen_adc : 在下面调用了一堆 tcg 的代码

- [ ] 关于 tcg 的描述，在 /home/maritns3/core/qemu/tcg/README

- [ ] /home/maritns3/core/qemu/tcg/i386/tcg-target.inc.c 被 tcg.c 直接 include 了

- [x] 似乎并没有打开 SOFTMMU, 但是这个依旧让 arm64 在 x86 的机器上运行。
  - SOFTMMU 对于系统态的模拟是必须的
  - 从 configure 的内容看，当 --target-list 中的名称含有 softmmu, 那么 softmmu 这个选项就会自动打开, 而不需要打开的位置是 user
  - 原因是 : user 的模拟只是一个普通的进程，qemu 在这个地址空间中间占有一部分位置而已

- [ ] /home/maritns3/core/qemu/accel/tcg/cputlb.c 都是干什么的

**WHEN COMMING BACK** 参考 niugen 的文档，然后调试分析吧。

some helper functions are called directly in translated code.

### target/arm/cpu.c

其实只是 cpu 相关的函数指针的初始化

- [ ] 可以借此分析一下 type_init 以及 TypeInfo 之类的东西
  - 比如 arm_cpu_class_init 就是给通用的各种函数指针注册 arm 对应的实现

### trace 
- [ ] 测试一下, 添加新的 Trace 的方法

- [ ]  为什么会支持各种 backend, 甚至有的 dtrace 的内容?

- [ ] 这里显示的 trace 都是做什么的 ?
```
➜  vn git:(master) ✗ qemu-system-x86_64 -trace help
```

- [ ] 例如这些文件:
/home/maritns3/core/qemu/hw/i386/trace-events
