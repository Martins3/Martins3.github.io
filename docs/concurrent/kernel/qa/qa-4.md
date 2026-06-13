## 经典案例，更新数组指针

从这里我们可以看到一个例子，只有一个位置需要设置 memory barrier ，而在另外一个
位置不需要。

看了注释才知道，`swap_info[type]` 本来就会对于会对于 type 形成依赖:
```c
static struct swap_info_struct *swap_info[MAX_SWAPFILES];


static struct swap_info_struct *alloc_swap_info(void)
{
		/*
		 * Publish the swap_info_struct after initializing it.
		 * Note that kvzalloc() above zeroes all its fields.
		 */
		smp_store_release(&swap_info[type], p); /* rcu_assign_pointer() */
		nr_swapfiles++;
}


static struct swap_info_struct *swap_type_to_swap_info(int type)
{
	if (type >= MAX_SWAPFILES)
		return NULL;

	return READ_ONCE(swap_info[type]); /* rcu_dereference() */
}
```

一个很长的解释，不过我不理解，为什么使用 rcu_dereference 和 rcu_assign_pointer 来类比，
等我理解完 rcu_dereference 再来说。
```diff
History:        #0
Commit:         a4b451143fa275a31f17a93adac3b8dbb3d20ca2
Author:         Ying Huang <huang.ying.caritas@gmail.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Tue 29 Jun 2021 10:37:09 AM CST
Committer Date: Wed 30 Jun 2021 01:53:49 AM CST

mm, swap: remove unnecessary smp_rmb() in swap_type_to_swap_info()

Before commit c10d38cc8d3e ("mm, swap: bounds check swap_info array
accesses to avoid NULL derefs"), the typical code to reference the
swap_info[] is as follows,

  type = swp_type(swp_entry);
  if (type >= nr_swapfiles)
          /* handle invalid swp_entry */;
  p = swap_info[type];
  /* access fields of *p.  OOPS! p may be NULL! */

Because the ordering isn't guaranteed, it's possible that swap_info[type]
is read before "nr_swapfiles".  And that may result in NULL pointer
dereference.

So after commit c10d38cc8d3e, the code becomes,

  struct swap_info_struct *swap_type_to_swap_info(int type)
  {
	  if (type >= READ_ONCE(nr_swapfiles))
		  return NULL;
	  smp_rmb();
	  return READ_ONCE(swap_info[type]);
  }

  /* users */
  type = swp_type(swp_entry);
  p = swap_type_to_swap_info(type);
  if (!p)
	  /* handle invalid swp_entry */;
  /* dereference p */

Where the value of swap_info[type] (that is, "p") is checked to be
non-zero before being dereferenced.  So, the NULL deferencing becomes
impossible even if "nr_swapfiles" is read after swap_info[type].
Therefore, the "smp_rmb()" becomes unnecessary.

And, we don't even need to read "nr_swapfiles" here.  Because the non-zero
checking for "p" is sufficient.  We just need to make sure we will not
access out of the boundary of the array.  With the change, nr_swapfiles
will only be accessed with swap_lock held, except in
swapcache_free_entries().  Where the absolute correctness of the value
isn't needed, as described in the comments.

We still need to guarantee swap_info[type] is read before being
dereferenced.  That can be satisfied via the data dependency ordering
enforced by READ_ONCE(swap_info[type]).  This needs to be paired with
proper write barriers.  So smp_store_release() is used in
alloc_swap_info() to guarantee the fields of *swap_info[type] is
initialized before swap_info[type] itself being written.  Note that the
fields of *swap_info[type] is initialized to be 0 via kvzalloc() firstly.
The assignment and deferencing of swap_info[type] is like
rcu_assign_pointer() and rcu_dereference().

Link: https://lkml.kernel.org/r/20210520073301.1676294-1-ying.huang@intel.com
```

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
