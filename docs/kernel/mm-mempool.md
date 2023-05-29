# mempool
使用 mempool 的目的:
The purpose of mempools is to help out in situations where a memory allocation must succeed, but sleeping is not an option. To that end, mempools pre-allocate a pool of memory and reserve it until it is needed. [^16]
[^16]: [kernel doc : Driver porting: low-level memory allocation](https://lwn.net/Articles/22909/)

```txt
#0  r1bio_pool_alloc (gfp_flags=600064, data=0xffff888103acffd0) at drivers/md/raid1.c:131
#1  0xffffffff8133c353 in mempool_alloc (pool=pool@entry=0xffff888130eb10a0, gfp_mask=601088, gfp_mask@entry=3072) at mm/mempool.c:398
#2  0xffffffff81e27e52 in alloc_r1bio (bio=0xffff88810c470100, mddev=0xffff888107ca2000) at drivers/md/raid1.c:1209
```

mempool_t::curr_nr : 现在还有多少存粮

- mempool_exit : 调用 mempool_t::free 接口将没有使用的元素的全部释放掉

- [ ] 如果存粮用玩了，如何?

## 如果一个正在 alloc ，但是正在调用 mempool_exit

- mempool_alloc
  - 使用特殊的 gfp_mask 首先分配一次，走标准路径
  - 如果失败，走 remove_element

在 mempool_init_node 预先分配内存

通过 mempool_t::lock 来实现互斥
