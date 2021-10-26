# QEMU 二进制翻译基础

<!-- vim-markdown-toc GitLab -->

- [unsorted](#unsorted)
- [abbreviation explanation](#abbreviation-explanation)
- [tb link](#tb-link)
- [TCGContext initialization](#tcgcontext-initialization)
- [cross page](#cross-page)

<!-- vim-markdown-toc -->

## unsorted
guest physical page 持有 tb 而不是 guest virtual page 持有 tb

## abbreviation explanation
- tb : Translation Block

## tb link
- [ ] xqm 是如何进行 tb link 的

## TCGContext initialization
```c
TCGContext tcg_init_ctx;
__thread TCGContext *tcg_ctx;

// 存放所有的 tls tcg_ctx
static TCGContext **tcg_ctxs;
static unsigned int n_tcg_ctxs;
```

tcg_ctx 是一个 tls 变量，其的初始化位置:
- tcg_context_init : 初始化 tcg_init_ctx
- tcg_register_thread : 将 tcg_init_tcx 直接拷贝

其中的 tcg_ctx 只会是 &tcg_init_tcx 的是:
- tcg_context_init
- code_gen_alloc
- alloc_code_gen_buffer
- tcg_prologue_init
- tcg_global_mem_new_internal

既会出现 &tcg_init_tcx 也会出现 vCPU tcg_ctx 的位置：
- temp_idx
- temp_tcgv_i32
- tcgv_i32_temp

也就是将 v4.2.1 (6cdf8c4efa0) 的位置打上一个 [patch](./tcg/tcg_ctx.patch) 之后 QEMU 依旧正常运行的

之所以会出 &tcg_init_tcx 和 vCPU tcg_ctx 都调用的函数，主要是因为在创建一些 global 的属性
- tcg_init => tcg_exec_init => cpu_gen_init => tcg_context_init
```c
    ts = tcg_global_reg_new_internal(s, TCG_TYPE_PTR, TCG_AREG0, "env");
    cpu_env = temp_tcgv_ptr(ts);
```
- tcg_x86_init

## cross page
[tb link](#tb-link) 大大加速了二进制翻译器的执行速度，但是在系统态会出现 cross page 的问题。

如果一个跳转从 guest virtual page A 上的 tb_1 跳转到 guest virtual page B 上的 tb_2，假设 tb_1 和 tb_2 没有 link 到一起:
- 从 page A 中的 tb_1 退出
- 使用 tb_lookup__cpu_state 查询失败
- tb_gen_code 生成 tb_2
- 将 tb_1 和 tb_2 链接起来

当下一次再次执行 tb_1 跳转到 tb_2 的时候，那么直接执行就可以了。

如果 B 被修改了，因为 SMC 机制，tb_2 将会被 invalidate 掉，所以不会出现问题。
但是通过修改 page table 的方式将 B 的映射的物理页面修改掉之后，因为 tb_1 和 tb_2
是通过 tb link 在一起的，tb_1 执行完成之后，直接跳转到 tb_2 上执行，这会跳过
tb_lookup__cpu_state 的查询过程。

正确的解决方法，不要让 cross page 的两个 tb link 在一起。

实际上 cross page 可以划分为两种情况:
1. tb A 跳转到 tb B 上，A B 分别在两个页面上
2. tb B 本身横跨两个页面


执行代码首先持有的是虚拟地址，在 tb_find => tb_lookup__cpu_state 中进行查找，可以保证 tb_jmp_cache 中找到的绝对有效，但是 TLB flush 之后，tb_jmp_cache 也是会被刷新的。
- 从 tb_find 看，其会处理第二种情况，就是直接不跳转的，但是对于第一种情况，没有处理，总是默认使用 tb_add_jump 添加上两者的
  - tb_set_jmp_target : 设置为跳转到下一个位置

- 当 cross page 的时候，cross 的是 virtual page
  - 虽然 virtual page 的 cross page 就是 physical page 的 cross page
  - QEMU 对于 corss page 的处理就是不做处理，所有跳转到 cross page 的页面都是只能退出，然后


```c
static inline bool use_goto_tb(DisasContext *s, target_ulong pc)
{
#ifndef CONFIG_USER_ONLY
    return (pc & TARGET_PAGE_MASK) == (s->base.tb->pc & TARGET_PAGE_MASK) ||
           (pc & TARGET_PAGE_MASK) == (s->pc_start & TARGET_PAGE_MASK);
#else
    return true;
#endif
}

static inline void gen_goto_tb(DisasContext *s, int tb_num, target_ulong eip)
{
    target_ulong pc = s->cs_base + eip;

    if (use_goto_tb(s, pc))  {
        /* jump to same page: we can use a direct jump */
        tcg_gen_goto_tb(tb_num);
        gen_jmp_im(s, eip);
        tcg_gen_exit_tb(s->base.tb, tb_num);
        s->base.is_jmp = DISAS_NORETURN;
    } else {
        /* jump to another page */
        gen_jmp_im(s, eip);
        gen_jr(s, s->tmp0);
    }
}
```
