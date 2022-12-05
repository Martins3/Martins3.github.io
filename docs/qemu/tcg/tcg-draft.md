# TCG related code flow analyze based on aarch64
- [ ] 模拟 和 qemu 执行的上下文切换在哪里呀 ?
- [ ] 靠什么规则查询 tb ?

- [ ] 从 capstone 到 arm, 似乎对理解 binary 的构成还是很有帮助的呀!
  - [ ] 似乎在 gen_intermediate_code 中间都是手动分析编码的，和 capstone 没有任何关系啊

- 如果没有 enable kvm 的话，x86_64 的机器上进行 x86_64 的翻译是怎么进行的 ？
  - 必然走 tcg 的流程
  - [ ] 但是不知道是否发生指令翻译的啊

- [x] 用户态的翻译是不是不需要各种地址加速的处理啊

- [ ] load store 的 softmmu 的 walk 在哪里?

- [ ] 如果在 tb 的执行中间发生了越过 tb 的跳转，需要进行 tb_find 吗 ?

- [ ] 需要验证的流程:
  - [ ] tb 的靠什么组织的，linked list ?
  - [ ] 当 tb 可以实现代码的时候，

- [ ] tcg 定义的规则指令是什么 ?

- [ ] 二进制翻译从哪里开始的，是不是 linker 和 loader 的功能也是需要实现的，是直接调用本地的 linker 和 loader 还是需要存在配套的 linker 和 loader ?
  - [ ] 从 arm 之前的机器看，只是需要从 linker 和 loader 只是需要本地的就可以了，但是具体 qemu 为什么可以知道需要模拟动态链接，暂时不清楚啊。

- [ ] qemu-aarch64-softmmu 执行的第一行代码在什么位置 ?

- [ ] cheng wei ren's notes

- [ ] 怎么检查中断 ? 中断是怎么模拟产生的 ?

- [x] 为什么二进制翻译需要考虑到内存一致性的问题
  - 如果处理器内存一致性的约束更弱，那么就会导致生成的代码比较随意了
  - 如果原本的按照 target 的代码生成，但是 host 的代码也是随意的生成，但是 host 代码无法保证其内容
  - 最简单的考虑，假如有了一对一的翻译。

## code coverage
- 在 accel/tcg 是 tcg
- tcg 应该是和 host 架构有关的代码生成工作
- ./softmmu 下面应该是整个 qemu 通用的入口了

## tcg find

## asynchronous events
accel/tcg/cpu-exec.c 中间的两个函数:
- cpu_handle_exception
- cpu_handle_interrupt

- [x] handle_exception 是同步的，为什么需要在 tb 函数的外面处理啊？
  - 当发生了 exception 之后，会直接从 tb 中间跳转出来，然后然让 qemu 设置 CPUState 之类的东西

## switch to and escape tb
- [x] stack 的切换在什么位置 ?
  - 根本不需要对于 stack 进行切换，因为 guest 的 stack 指针实际上在 CPUState 上才对
  - guset 的寄存器是显然不需要 qemu 进行分配的
    - [ ] 但是 host 的寄存器的分配是怎么回事 ?
      - 毕竟在执行 host 代码, 生成的是 host 的汇编
      - host 在于模拟这些工作

- [x] 为什么需要 0x488 的 stack
  - tcg_set_frame && tcg_func_start
  - [ ] tcg 会调用一些 helper 函数，以及使用一些临时变量，这些都需要占用空间

1. 在 tcg_prologue_init 中创建 switch to 的机制:
  - tcg_target_qemu_prologue

```c
PROLOGUE: [size=45]
0x7f1e5c000000:  55                       pushq    %rbp
0x7f1e5c000001:  53                       pushq    %rbx
0x7f1e5c000002:  41 54                    pushq    %r12
0x7f1e5c000004:  41 55                    pushq    %r13
0x7f1e5c000006:  41 56                    pushq    %r14
0x7f1e5c000008:  41 57                    pushq    %r15
0x7f1e5c00000a:  48 8b ef                 movq     %rdi, %rbp
0x7f1e5c00000d:  48 81 c4 78 fb ff ff     addq     $-0x488, %rsp
0x7f1e5c000014:  ff e6                    jmpq     *%rsi
0x7f1e5c000016:  33 c0                    xorl     %eax, %eax # 有的是这个入口, 返回值为 0 : code_gen_epilogue
0x7f1e5c000018:  48 81 c4 88 04 00 00     addq     $0x488, %rsp # 有的是真的需要返回值的 : tb_ret_addr
0x7f1e5c00001f:  c5 f8 77                 vzeroupper
0x7f1e5c000022:  41 5f                    popq     %r15
0x7f1e5c000024:  41 5e                    popq     %r14
0x7f1e5c000026:  41 5d                    popq     %r13
0x7f1e5c000028:  41 5c                    popq     %r12
0x7f1e5c00002a:  5b                       popq     %rbx
0x7f1e5c00002b:  5d                       popq     %rbp
0x7f1e5c00002c:  c3                       retq
```

- [ ] 根本没有考虑到 xmm 指令之类的保存，还是在 guest 中间的 simd 都是不经过模拟的
- [ ] 对于一些跳转，当生成的时候无法找到的时候，那么应该将这个跳转指向 exit 到 host 中间

## chain

- target/arm/translate-a64.c::disas_uncond_b_imm : 从 translator_loop 一路都是译码的工作
  - tcg_gen_lookup_and_goto_ptr
    - `HELPER(lookup_tb_ptr)(CPUArchState *env)`  : 如果可以根据查询到下一个位置, 那么就 link 起来, 否则返回 `tcg_ctx->code_gen_epilogue`
      - tb_lookup__cpu_state
    - tcg_gen_exit_tb : 产生一条 INDEX_op_exit_tb 的中间指令, 在 tcg_out_op 中间生成对应的 host 代码
      - `tcg_out_jmp(s, s->code_gen_epilogue);`
      - tcg_out_jmp(s, tb_ret_addr);

- [ ] tb 的跳转地址实际在 target 的代码上是确定的，但是在 host 上，这些地址取决于 tb 的 hva 了

- [ ] 是如何处理 tcg_qemu_tb_exec 的返回值的 ?
  - [ ] 返回值为 0, 1, 2 表示什么 ?
  - [ ] 关注返回值为 3 如何处理 interrupt 的
  - [x] synchronize_from_tb 是干什么的 ? 让代码从 TB 开始的位置执行
  - [x] 为什么需要返回 last_tb ?
    - 在 tb_find 中找到下一个跳转位置的后，可以会修改 last_tb 中的跳转位置

- [ ] 为什么可以 chain 的，比如对于一个 switch 函数，target 的一个跳转指令，实际上是可以跳转到多个 tb 的, 通过设置 tb 的跳转位置有意义吗 ?
  - 每一个 tb 中间只有一个跳转指令, 就算是 switch，跳转实际上被拆分为多个 tb 了
  - 类似于 ret 指令，实际上，函数从多个位置调用，ret 实际上并不知道从哪一个位置返回, 所以，猜测函数的处理会比较特殊啊
  - 其实大多数时候，一个 tb 的跳转位置都是确定的, 至于选择那一条道路，这是 native code 执行的时候决定的

## interrupt
- 如果每一个 tb 都去检查 interrupt, 显然非常的不合逻辑
- [ ] 但是检查的时机是什么啊 ?

实现的内容 : tb_find

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
    - tb_gen_code : 在这里初始化了 TranslationBlock
      - `get_page_addr_code` : get the `phys_pc` according to `pc`
        - [ ] 这里会进入到 cputlb.c 中间，如果这里也是需要进行 TLB 翻译的，和 gencode 时候的 TLB 翻译的关系是什么 ?
      - gen_intermediate_code : 在文件夹 target/arm 下面, 这个短短的函数用于选择到底是 arm 还是 arm64 的 TranslatorOps.
      - tcg_gen_code : 最终的代码会出现在 qemu/tcg/aarch64/tcg-target.inc.c 中间
        - tcg_reg_alloc_op
          - tcg_out_op
            - tcg_out_qemu_st
              - tcg_out_tlb_load : 必然找到 TLB 的代码
              - tcg_out_qemu_st_direct : 完成 store 操作
      - tb_link_page : set this TB's related physical page
  - cpu_loop_exec_tb
    - cpu_tb_exec
      - tcg_qemu_tb_exec

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

some helper functions are called directly in translated code.

- [ ] softmmu 中间到底存储了什么东西啊 ?
  - [ ] 真的是对于所有的 load store 指令都是需要 softmmu page walk 一下，还是说，当遇到新的 page 需要如此 ?

- [x] label_ptr 的作用，为什么需要一个大小为 2 的数组 ?
  - 因为 x86 的跳转指令可能是大于 32bit
  - 在 arm64 中间就不存在这个问题
  - label_ptr 记录的是 jcc 指令的位置，`s->code_ptr`

- tcg_out_ldst_finalize 最终设置 jcc 指令需要跳转的位置为 : tcg_out_qemu_st_slow_path 生成的代码
  - 这个生成的代码是为了组装参数
  - [ ] 这个跳转代码为什么需要动态生成啊 ?

```c
// include/exec/cpu-defs.h

typedef struct CPUTLBEntry {
    union {
        struct {
            target_ulong addr_read;
            target_ulong addr_write;
            target_ulong addr_code;
            /* Addend to virtual address to get host address.  IO accesses
               use the corresponding iotlb value.  */
            uintptr_t addend;
        };
        /* padding to get a power of two size */
        uint8_t dummy[1 << CPU_TLB_ENTRY_BITS];
    };
} CPUTLBEntry;
```

### target/arm/cpu.c
其实只是 cpu 相关的函数指针的初始化

- [ ] 可以借此分析一下 type_init 以及 TypeInfo 之类的东西
  - 比如 arm_cpu_class_init 就是给通用的各种函数指针注册 arm 对应的实现

[^1]: https://stackoverflow.com/questions/48139513/asm-x86-64-avx-xmm-and-ymm-registers-differences
