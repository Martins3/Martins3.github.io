# qht 移植

- 在 tb_link_page 中 qht_insert 什么时候无法插入啊，是不是在 single core 的模式下，可以保证总是插入成功的
  - 既然进入到 tb_link_page 的位置，说明刚才还是无法找到这个 tb 的
  - 之前一直以为是 tb_link_page 返回是因为 tb_page_add 造成的，现在才发现实际上是因为 qht_insert 造成的
  - 向 tb_page_add 上添加的时候是不会比较的，但是 qht_insert 失败之后，需要会删除

## 问题
- 不能将 call_rcu 可以变为一个空函数吧，应该修改为直接释放吧

- [ ] qht_cmp_func_t 和 qht_lookup_func_t 分别是用来干啥的
  - 分别对应 tb_cmp 和 tb_lookup_cmp

- [ ] 为什么一个 hash table 会需要 cmp 啊，都是放到一个 buket 不就可以了吗 ?

```c
static bool tb_cmp(const void *ap, const void *bp)
{
    const TranslationBlock *a = ap;
    const TranslationBlock *b = bp;

    return a->pc == b->pc &&
        a->cs_base == b->cs_base &&
        a->flags == b->flags &&
        (tb_cflags(a) & CF_HASH_MASK) == (tb_cflags(b) & CF_HASH_MASK) &&
        a->trace_vcpu_dstate == b->trace_vcpu_dstate &&
        a->page_addr[0] == b->page_addr[0] &&
        a->page_addr[1] == b->page_addr[1];
}

static bool tb_lookup_cmp(const void *p, const void *d)
{
    const TranslationBlock *tb = p;
    const struct tb_desc *desc = d;

    if (tb->pc == desc->pc &&
        tb->page_addr[0] == desc->phys_page1 &&
        tb->cs_base == desc->cs_base &&
        tb->flags == desc->flags &&
        tb->trace_vcpu_dstate == desc->trace_vcpu_dstate &&
        (tb_cflags(tb) & (CF_HASH_MASK | CF_INVALID)) == desc->cf_mask) {
        /* check next page if needed */
        if (tb->page_addr[1] == -1) {
            return true;
        } else {
            tb_page_addr_t phys_page2;
            target_ulong virt_page2;

            virt_page2 = (desc->pc & TARGET_PAGE_MASK) + TARGET_PAGE_SIZE;
            phys_page2 = get_page_addr_code(desc->env, virt_page2);
            if (tb->page_addr[1] == phys_page2) {
                return true;
            }
        }
    }
    return false;
}
```

- [ ] 在 tb_lookup__cpu_state 也是存在比较函数的
  - 还引入了一个痛苦面具的 CF_CLUSTER_SHIFT
```c
    cf_mask &= ~CF_CLUSTER_MASK;
    cf_mask |= cpu->cluster_index << CF_CLUSTER_SHIFT;
```
- [ ] tb_cflags ：为什么 cflags 需要使用 atomic 访问啊
- [ ] trace_vcpu_dstate 做什么用的
- [ ] tb_lookup_cmp 中比较 flags 的时候需要增加一个 CF_INVALID 的内容

## 到底在比较什么内容
1. TranslationBlock::pc : 所有就是 guest virtual address

- tb_find
  - tb_lookup__cpu_state
    - cpu_get_tb_cpu_state
      - cpu_get_tb_cpu_state
        - `*pc = *cs_base + env->eip;` : 构造位置
  - tb_gen_code : 赋值位置

why : 什么时候出现，两个 tb 物理地址相同，但是 pc 不同的时候
  - 比较 pc，在进程 m 中创建出来的 pc 表示


## [ ] qht_lookup_custom 的几个参数的含义是什么
```c
void *qht_lookup(const struct qht *ht, const void *userp, uint32_t hash)
{
    return qht_lookup_custom(ht, userp, hash, ht->cmp);
}
```

使用 qht_cmp_func_t 来实现 TranslationBlock 的*排序*
> 恐怕并不是，只是比较而已

而使用 qht_lookup_func_t 来实现比较的，那么只能采用
几乎所有的都是使用 qht_lookup 的，除了 tb 的查询 qht_lookup_custom 的
> 的确是因为比较的两个内容不同造成的

## 分析一下 qht.c 的实现
1. seqlock 是干啥的?
