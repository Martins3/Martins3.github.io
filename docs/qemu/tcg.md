## cross page tb

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

是 guest physical page 持有 tb 而不是 guest virtual page 持有 tb

执行代码首先持有的是虚拟地址，在 tb_find => tb_lookup__cpu_state 中进行查找，可以保证 tb_jmp_cache 中找到的绝对有效，但是 TLB flush 之后，tb_jmp_cache 也是会被刷新的。


存在两种情况的 cross page，
1. tb A 跳转到 tb B 上，A B 分别在两个页面上
2. tb B 本身横跨两个页面

- [ ] 从 tb_find 看，其会处理第二种情况，就是直接不跳转的，但是对于第一种情况，没有处理，总是默认使用 tb_add_jump 添加上两者的
  - [ ] 找不到证据说明会处理第一种情况啊
  - tb_set_jmp_target : 设置为跳转到下一个位置


- 当 cross page 的时候，cross 的是 virtual page
  - 虽然 virtual page 的 cross page 就是 physical page 的 cross page
  - QEMU 对于 corss page 的处理就是不做处理，所有跳转到 cross page 的页面都是只能退出，然后

xqm 增加的处理:

- do_tb_flush
  - CPU_FOREACH(cpu) { cpu_tb_jmp_cache_clear(cpu); }
    - xtm_pf_inc_jc_clear
    - *xtm_cpt_flush* : clear Code Page Table (cpt)
    - `atomic_set(&cpu->tb_jmp_cache[i], NULL);` : 就是直接 tb_jmp_cache 的吗? 难道不会采用更加复杂的东西吗 ?
- tb_lookup__cpu_state
  - *xtm_cpt_insert_tb*
- tb_jmp_cache_clear_page
  - *xtm_cpt_flush_page*
